#include "ant_scanner.h"
#include "ant_interface.h"
#include "ant_parameters.h"
#include "ant_channel_config.h"
#include "ant_error.h"
#include "nrf_sdh_ant.h"
#include "nrf_log.h"
#include "common_definitions.h"
#include "app_timer.h"
#include "nrf_delay.h"

#define ANTPLUS_NETWORK_NUMBER     0  // Use network 0
#define SCAN_CHANNEL_NUMBER        0  // MUST be 0 for wildcard scan, all other channels must be closed
#define SCAN_CHANNEL_PERIOD        8192   // 4 Hz (8192 counts = 4.00 Hz)
#define SCAN_RF_FREQUENCY          57     // 2457 MHz (standard ANT+ frequency)

// Structure to store found devices
typedef struct {
    uint16_t device_id;
    int8_t rssi;
} ant_device_t;

// Static variables
static ant_device_callback_t m_device_callback = NULL;
static ant_device_t m_found_devices[MAX_ANT_DEVICES];
static uint8_t m_num_found_devices = 0;
static bool m_scanning_active = false;
static bool m_channels_closing = false;
static uint8_t m_channels_to_close = 0;
APP_TIMER_DEF(m_scan_timer);
static bool m_timer_created = false;

// Forward declarations
static void scan_timer_handler(void *p_context);
static void scan_ant_evt_handler(ant_evt_t * p_ant_evt, void * p_context);

// Register ANT event handler
NRF_SDH_ANT_OBSERVER(m_ant_scanner_observer,
                     APP_ANT_OBSERVER_PRIO,
                     scan_ant_evt_handler,
                     NULL);

/**@brief Timer handler to stop scanning after timeout */
static void scan_timer_handler(void *p_context)
{
    if (m_scanning_active)
    {
        NRF_LOG_INFO("Scan timeout reached, stopping scanner...");
        m_scanning_active = false;  // Set this first to prevent re-entry
        ant_scanner_stop();  // This will close channels and clean up
        nrf_delay_ms(100);  // Give time for cleanup
        
        // Notify that scanning has stopped due to timeout
        if (m_device_callback != NULL)
        {
            m_device_callback(0, 0);  // Send a special event to indicate timeout
        }
    }
}

/**@brief Check if a device is already in the found devices list */
static bool is_device_in_list(uint16_t device_id)
{
    for (uint8_t i = 0; i < m_num_found_devices; i++)
    {
        if (m_found_devices[i].device_id == device_id)
        {
            return true;
        }
    }
    return false;
}

/**@brief Add a new device to the found devices list */
static void add_device(uint16_t device_id, int8_t rssi)
{
    if (m_num_found_devices < MAX_ANT_DEVICES)
    {
        m_found_devices[m_num_found_devices].device_id = device_id;
        m_found_devices[m_num_found_devices].rssi = rssi;
        m_num_found_devices++;
        
        // Call the registered callback if any
        if (m_device_callback != NULL)
        {
            m_device_callback(device_id, rssi);
        }
    }
}

/**@brief ANT event handler for wildcard scan channel */
static void scan_ant_evt_handler(ant_evt_t * p_ant_evt, void * p_context)
{
    if (m_channels_closing)
    {
        if (p_ant_evt->event == EVENT_CHANNEL_CLOSED)
        {
            m_channels_to_close--;
            NRF_LOG_INFO("Channel %u closed. %u channels remaining", p_ant_evt->channel, m_channels_to_close);
            
            if (m_channels_to_close == 0)
            {
                m_channels_closing = false;
                // All channels are closed, now we can start scanning
                uint32_t err_code = sd_ant_rx_scan_mode_start(0);
                if (err_code != NRF_SUCCESS)
                {
                    NRF_LOG_ERROR("Failed to start scan mode: 0x%08X", err_code);
                    m_scanning_active = false;  // Ensure we don't get stuck in scanning state
                    return;
                }
                
                // Start scan timeout timer
                err_code = app_timer_start(m_scan_timer, APP_TIMER_TICKS(MAX_SCAN_DURATION_MS), NULL);
                if (err_code != NRF_SUCCESS)
                {
                    NRF_LOG_ERROR("Failed to start scan timer: 0x%08X", err_code);
                    m_scanning_active = false;  // Ensure we don't get stuck in scanning state
                    return;
                }

                m_scanning_active = true;
                NRF_LOG_INFO("ANT Scanner: Started scanning");
            }
        }
        return;
    }

    if (p_ant_evt->channel != SCAN_CHANNEL_NUMBER)
    {
        return;
    }

    switch (p_ant_evt->event)
    {
        case EVENT_RX:
        {
            ANT_MESSAGE * p_ant_message = (ANT_MESSAGE *)&p_ant_evt->message;

            if (p_ant_message->ANT_MESSAGE_stExtMesgBF.bANTDeviceID &&
                p_ant_message->ANT_MESSAGE_stExtMesgBF.bANTRssi)
            {
                uint8_t * p_ext = p_ant_message->ANT_MESSAGE_aucExtData;
                uint16_t dev_number = (uint16_t)(p_ext[0] | (p_ext[1] << 8));
                uint8_t  dev_type   = p_ext[2];
                int8_t   rssi       = (int8_t)p_ext[5];
            
                if (dev_type == 11 && !is_device_in_list(dev_number))
                {
                    add_device(dev_number, rssi);
                    NRF_LOG_INFO("Found ANT+ device: ID=%u, RSSI=%d dBm", dev_number, rssi);
                }
            }
        }
        break;

        case EVENT_CHANNEL_CLOSED:
        {
            NRF_LOG_INFO("ANT Scanner: Channel %u closed.", SCAN_CHANNEL_NUMBER);
            m_scanning_active = false;  // Ensure we're not in scanning state anymore
        }
        break;

        default:
            break;
    }
}

uint32_t ant_scanner_init(ant_device_callback_t callback)
{
    uint32_t err_code;

    m_device_callback = callback;

    // Create timer only once
    if (!m_timer_created)
    {
        err_code = app_timer_create(&m_scan_timer, APP_TIMER_MODE_SINGLE_SHOT, scan_timer_handler);
        if (err_code != NRF_SUCCESS)
        {
            NRF_LOG_ERROR("Failed to create ANT scan timer: 0x%08X", err_code);
            return err_code;
        }
        m_timer_created = true;
    }

    // Close all channels first (ANT+ requirement for scanning)
    for (uint8_t channel = 0; channel < 5; channel++)  // ANT+ supports up to 2 channels
    {
        err_code = sd_ant_channel_close(channel);
        if (err_code != NRF_SUCCESS && err_code != NRF_ERROR_INVALID_PARAM)
        {
            NRF_LOG_WARNING("Failed to close channel (Probably expected) %d: 0x%08X", channel, err_code);
        }
    }
    // Set ANT+ Network Key
    err_code = sd_ant_network_address_set(ANTPLUS_NETWORK_NUMBER, ANT_PLUS_NETWORK_KEY);
    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }

    // Assign ANT channel as slave (receiver)
    err_code = sd_ant_channel_assign(SCAN_CHANNEL_NUMBER,
                                   CHANNEL_TYPE_SLAVE,
                                   ANTPLUS_NETWORK_NUMBER,
                                   0x01);  // Background scanning
    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }

    // Set radio frequency
    err_code = sd_ant_channel_radio_freq_set(SCAN_CHANNEL_NUMBER, SCAN_RF_FREQUENCY);
    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }

    // Set channel period
    err_code = sd_ant_channel_period_set(SCAN_CHANNEL_NUMBER, SCAN_CHANNEL_PERIOD);
    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }

    // Enable extended data: Device ID, Device Type, Transmission Type, RSSI
    err_code = sd_ant_lib_config_set(
        ANT_LIB_CONFIG_MESG_OUT_INC_DEVICE_ID |  // 0x80 (device ID)
        ANT_LIB_CONFIG_MESG_OUT_INC_RSSI         // 0x40 (RSSI)
    );
    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }

    NRF_LOG_INFO("ANT Scanner: Service initialized");
    return NRF_SUCCESS;
}

uint32_t ant_scanner_start(void)
{
    if (m_scanning_active || m_channels_closing)
    {
        return NRF_ERROR_INVALID_STATE;
    }

    uint32_t err_code;

    // Clear found devices list
    m_num_found_devices = 0;
    memset(m_found_devices, 0, sizeof(m_found_devices));

    // Close all channels first (ANT+ requirement for scanning)
    m_channels_closing = true;
    m_channels_to_close = 0;
    
    for (uint8_t channel = 0; channel < 5; channel++)  // ANT+ supports up to 2 channels
    {
        err_code = sd_ant_channel_close(channel);
        if (err_code == NRF_SUCCESS)
        {
            m_channels_to_close++;
        }
        else if (err_code != NRF_ERROR_INVALID_PARAM)
        {
            NRF_LOG_WARNING("Failed to close channel %d: 0x%08X", channel, err_code);
        }
    }

    if (m_channels_to_close == 0)
    {
        // No channels needed closing, start scanning immediately
        m_channels_closing = false;
        err_code = sd_ant_rx_scan_mode_start(0);
        if (err_code != NRF_SUCCESS)
        {
            return err_code;
        }

        // Start scan timeout timer
        err_code = app_timer_start(m_scan_timer, APP_TIMER_TICKS(MAX_SCAN_DURATION_MS), NULL);
        if (err_code != NRF_SUCCESS)
        {
            err_code = sd_ant_channel_close(SCAN_CHANNEL_NUMBER);  // Stop scanning
            APP_ERROR_CHECK(err_code);
            return err_code;
        }

        m_scanning_active = true;
        NRF_LOG_INFO("ANT Scanner: Started scanning");
    }

    return NRF_SUCCESS;
}

void ant_scanner_stop(void)
{
    if (!m_scanning_active && !m_channels_closing)
    {
        return;  // Already stopped
    }

    // Stop the scan timeout timer
    app_timer_stop(m_scan_timer);

    // Stop background scanning mode
    uint32_t err_code = sd_ant_channel_close(SCAN_CHANNEL_NUMBER);  // Stop scanning
    if (err_code != NRF_SUCCESS)
    {
        NRF_LOG_WARNING("Failed to stop scanning mode: 0x%08X", err_code);
    }

    m_scanning_active = false;
    m_channels_closing = false;
    m_device_callback = NULL;
    NRF_LOG_INFO("ANT Scanner: Stopped scanning");
}

bool ant_scanner_is_active(void)
{
    return m_scanning_active;
}
