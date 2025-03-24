#include "ant_scanner.h"
#include "ant_interface.h"
#include "ant_parameters.h"
#include "ant_channel_config.h"
#include "ant_error.h"
#include "nrf_sdh_ant.h"
#include "nrf_log.h"
#include "common_definitions.h"

#define ANTPLUS_NETWORK_NUMBER     0  // Use network 0
#define SCAN_CHANNEL_NUMBER        0
#define SCAN_CHANNEL_PERIOD        8192   // 4 Hz (8192 counts = 4.00 Hz)
#define SCAN_RF_FREQUENCY          57     // 2457 MHz (standard ANT+ frequency)

static void scan_ant_evt_handler(ant_evt_t * p_ant_evt, void * p_context);

NRF_SDH_ANT_OBSERVER(m_ant_scanner_observer,
                     APP_ANT_OBSERVER_PRIO,
                     scan_ant_evt_handler,
                     NULL);

/**@brief Function to configure wildcard scanning channel. */
void ant_scanner_init(void)
{
    uint32_t err_code;

    // Set ANT+ Network Key (for network 0)
    err_code = sd_ant_network_address_set(ANTPLUS_NETWORK_NUMBER, ANT_PLUS_NETWORK_KEY);
    APP_ERROR_CHECK(err_code);

    // Assign ANT channel as slave (receiver), extended assignment = 0x01 (background scanning)
    err_code = sd_ant_channel_assign(SCAN_CHANNEL_NUMBER,
                                     CHANNEL_TYPE_SLAVE,
                                     ANTPLUS_NETWORK_NUMBER,
                                     0x01);
    APP_ERROR_CHECK(err_code);

    // Set radio frequency
    err_code = sd_ant_channel_radio_freq_set(SCAN_CHANNEL_NUMBER, SCAN_RF_FREQUENCY);
    APP_ERROR_CHECK(err_code);

    // Set channel period
    err_code = sd_ant_channel_period_set(SCAN_CHANNEL_NUMBER, SCAN_CHANNEL_PERIOD);
    APP_ERROR_CHECK(err_code);

    // Enable extended data: Device ID, Device Type, Transmission Type, RSSI
    err_code = sd_ant_lib_config_set(
        ANT_LIB_CONFIG_MESG_OUT_INC_DEVICE_ID |  // 0x80 (device ID)
        ANT_LIB_CONFIG_MESG_OUT_INC_RSSI         // 0x40 (RSSI)
    );

    if (err_code != NRF_SUCCESS)
    {
        NRF_LOG_ERROR("Failed ANT lib config: %lu", err_code);
    }
    APP_ERROR_CHECK(err_code);

    // Start background scanning mode
    err_code = sd_ant_rx_scan_mode_start(SCAN_CHANNEL_NUMBER);
    APP_ERROR_CHECK(err_code);

    NRF_LOG_INFO("ANT Scanner: Wildcard scanning initialized on channel %u.", SCAN_CHANNEL_NUMBER);
}

/**@brief ANT event handler for wildcard scan channel. */
static void scan_ant_evt_handler(ant_evt_t * p_ant_evt, void * p_context)
{
    char info_str[256];

    if (p_ant_evt->channel != SCAN_CHANNEL_NUMBER)
    {
        snprintf(info_str, sizeof(info_str),
                 "Ignoring ANT evt on ch=%u (wanted=%u), evt=%u",
                 p_ant_evt->channel, SCAN_CHANNEL_NUMBER, p_ant_evt->event);
        NRF_LOG_INFO("%s", info_str);
        return;
    }

    switch (p_ant_evt->event)
    {
        case EVENT_RX:
        {
            const uint8_t * p_payload = p_ant_evt->message.stMessage
                                              .uFramedData.stFramedData
                                              .uMesgData.stMesgData.aucPayload;

            const uint8_t * p_ext = p_ant_evt->message.stMessage
                                           .uFramedData.stFramedData
                                           .uMesgData.stMesgData.aucExtData;

            // Parse extended data for device info
            uint16_t dev_number = (uint16_t)(p_ext[0] | (p_ext[1] << 8));
            uint8_t  dev_type   = p_ext[2];
            uint8_t  trans_type = p_ext[3];
            int8_t   rssi       = (int8_t)p_ext[5]; 

            // Prepare formatted string for logging all information
            snprintf(info_str, sizeof(info_str),
                     "ANT RX Ch=%u Dev=%u Type=%u Trans=%u RSSI=%ddBm Payload=[%02X %02X %02X %02X %02X %02X %02X %02X]",
                     p_ant_evt->channel,
                     dev_number, dev_type, trans_type, rssi,
                     p_payload[0], p_payload[1], p_payload[2], p_payload[3],
                     p_payload[4], p_payload[5], p_payload[6], p_payload[7]);

            NRF_LOG_INFO("%s", info_str);
        }
        break;

        case EVENT_RX_SEARCH_TIMEOUT:
        {
            NRF_LOG_INFO("ANT Scanner: Search timeout on channel %u.", SCAN_CHANNEL_NUMBER);
        }
        break;

        case EVENT_CHANNEL_CLOSED:
        {
            NRF_LOG_INFO("ANT Scanner: Channel %u closed.", SCAN_CHANNEL_NUMBER);
        }
        break;

        default:
        {
            snprintf(info_str, sizeof(info_str),
                     "ANT Scanner: Unhandled event %u on channel %u.",
                     p_ant_evt->event, p_ant_evt->channel);
            NRF_LOG_INFO("%s", info_str);
        }
        break;
    }
}
