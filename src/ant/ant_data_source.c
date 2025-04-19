/**
 * @file ant_data_source.c
 * @brief Implementation of the ANT+ Data Source
 */

#include "ant_data_source.h"
#include "includes/cycling_data_model.h"
#include "common_definitions.h"

#include "nrf_sdh_ant.h"
#include "ant_parameters.h"
#include "ant_interface.h"
#include "ant_bpwr.h"
#include "app_timer.h"
#include "nrf_log.h"
#include "app_error.h"
#include "bsp.h"
#include "includes/ble_bridge.h"

// ANT+ BPWR profile instance
static ant_bpwr_profile_t m_ant_bpwr;

// ANT event handlers
static void ant_evt_handler(ant_evt_t * p_ant_evt, void * p_context);
static void ant_bpwr_evt_handler(ant_bpwr_profile_t * p_profile, ant_bpwr_evt_t event);
static void ant_bpwr_disp_evt_handler_filtered(ant_evt_t * p_ant_evt, void * p_context);

// ANT channel active flag
static bool m_ant_active = false;

// ANT+ restart timer
APP_TIMER_DEF(m_ant_restart_timer);

// Data source callbacks
static data_update_callback_t m_data_callback = NULL;
static uint16_t m_device_id = 0;

// Register the ANT event handlers
NRF_SDH_ANT_OBSERVER(m_ant_observer, APP_ANT_OBSERVER_PRIO, ant_evt_handler, NULL);
NRF_SDH_ANT_OBSERVER(m_ant_bpwr_observer, ANT_BPWR_ANT_OBSERVER_PRIO, ant_bpwr_disp_evt_handler_filtered, &m_ant_bpwr);

// Display callback structure
static ant_bpwr_disp_cb_t m_ant_bpwr_disp_cb;

// ANT+ BPWR profile configuration
static const ant_bpwr_disp_config_t m_ant_bpwr_profile_bpwr_disp_config = {
    .p_cb        = &m_ant_bpwr_disp_cb,
    .evt_handler = ant_bpwr_evt_handler,
};

/**
 * @brief Timer handler for ANT+ restart
 */
static void ant_restart_timer_handler(void *p_context) {
    NRF_LOG_INFO("🔄 Timer expired - Restarting ANT+...");
    uint32_t err_code = ant_bpwr_disp_open(&m_ant_bpwr);
    if (err_code != NRF_SUCCESS) {
        NRF_LOG_ERROR("🚨 ant_bpwr_disp_open FAILED: 0x%08X", err_code);
        return;
    }
    m_ant_active = true;
    NRF_LOG_INFO("✅ ant_bpwr_disp_open SUCCESS!");
}

/**
 * @brief Custom filtering wrapper for ANT+ events
 */
static void ant_bpwr_disp_evt_handler_filtered(ant_evt_t * p_ant_evt, void * p_context) {
    if (p_ant_evt->channel == ANT_BPWR_ANT_CHANNEL) {
        // Only forward events from ANT_BPWR_ANT_CHANNEL to Nordic's handler
        ant_bpwr_disp_evt_handler(p_ant_evt, p_context);
    } else {
        // Optional debug log
        NRF_LOG_DEBUG("Ignored ANT event on channel %u in BPWR handler (expected %u)",
                     p_ant_evt->channel, ANT_BPWR_ANT_CHANNEL);
    }
}

/**
 * @brief ANT+ BPWR event handler
 */
static void ant_bpwr_evt_handler(ant_bpwr_profile_t * p_profile, ant_bpwr_evt_t event) {
    if (p_profile == NULL) {
        NRF_LOG_ERROR("🚨 ERROR: p_profile is NULL!");
        return;
    }

    switch (event) {
        case ANT_BPWR_PAGE_16_UPDATED: {
            uint16_t power = p_profile->page_16.instantaneous_power;
            uint8_t cadence = p_profile->common.instantaneous_cadence;
            
            NRF_LOG_DEBUG("🚴 Raw Power: %d W, Cadence: %d RPM", power, cadence);
            
            // Call the data update callback
            if (m_data_callback != NULL) {
                m_data_callback(power, cadence);
            }
            break;
        }
        
        case ANT_BPWR_PAGE_80_UPDATED:  // Manufacturer Info (Page 80)
        {
            uint16_t manufacturer_id = p_profile->page_80.manufacturer_id;
            uint16_t model_number = p_profile->page_80.model_number;
            uint8_t hardware_revision = p_profile->page_80.hw_revision;

            NRF_LOG_INFO("📋 Manufacturer Info - HW Rev: %d, Manufacturer: %d, Model: %d",
                         hardware_revision, manufacturer_id, model_number);
            break;
        }

        case ANT_BPWR_PAGE_81_UPDATED:  // Product Information (Page 81)
        {
            uint8_t sw_revision_main = p_profile->page_81.sw_revision_major;
            uint8_t sw_revision_supplemental = p_profile->page_81.sw_revision_minor;
            uint32_t serial_number = p_profile->page_81.serial_number;

            uint16_t software_version = (sw_revision_supplemental == 0xFF) ?
                                        sw_revision_main :
                                        (sw_revision_main * 100) + sw_revision_supplemental;

            NRF_LOG_INFO("📋 Product Info - SW Version: %d, Serial Number: %u",
                         software_version, serial_number);
            break;
        }

        default:
            NRF_LOG_WARNING("⚠️ Unknown ANT+ Page Update: %d", event);
            break;
    }
}

/**
 * @brief ANT+ event handler
 */
void ant_evt_handler(ant_evt_t * p_ant_evt, void * p_context) {
    // Ignore events from any channel except the Bike Power Channel
    if (p_ant_evt->channel != ANT_BPWR_ANT_CHANNEL) {
        NRF_LOG_DEBUG("Ignoring ANT event from channel %u (expected channel %u)",
                     p_ant_evt->channel, ANT_BPWR_ANT_CHANNEL);
        return;
    }

    // Flash LED for any ANT+ message received (in debug mode)
    if (p_ant_evt->event == EVENT_RX) {
        #if defined(DEBUG) && !defined(RELEASE) 
            bsp_board_led_invert(3);       // Toggle LED4
        #endif
    }

    switch (p_ant_evt->event) {
        case EVENT_RX:
            m_ant_active = true;
            break;

        case EVENT_RX_SEARCH_TIMEOUT:
        case EVENT_CHANNEL_CLOSED:
            m_ant_active = false;
            NRF_LOG_WARNING("⚠️ ANT+ channel closed or search timeout");
            // Notify BLE Bridge that data source is lost
            ble_bridge_data_source_lost();
            break;

        case EVENT_RX_FAIL:
            NRF_LOG_WARNING("⚠️ ANT+ RX Fail: %d", p_ant_evt->event);
            break;

        case EVENT_RX_DATA_OVERFLOW:
            NRF_LOG_WARNING("⚠️ ANT+ RX Data Overflow: %d", p_ant_evt->event);
            break;

        default:
            NRF_LOG_DEBUG("⚠️ Unhandled ANT+ event: %d", p_ant_evt->event);
            break;
    }
}

/**
 * @brief Initialize the ANT+ data source
 */
static bool ant_source_init(data_source_config_t* config) {
    uint32_t err_code;
    
    if (config == NULL || config->type != DATA_SOURCE_ANT_PLUS) {
        NRF_LOG_ERROR("Invalid ANT+ data source configuration");
        return false;
    }
    
    // Store the configuration
    m_device_id = config->device_id;
    m_data_callback = config->data_callback;
    
    // Create the ANT+ restart timer
    err_code = app_timer_create(&m_ant_restart_timer, APP_TIMER_MODE_SINGLE_SHOT, ant_restart_timer_handler);
    APP_ERROR_CHECK(err_code);
    
    NRF_LOG_INFO("ANT+ Data Source: Initialized with device ID %d", m_device_id);
    
    return true;
}

/**
 * @brief Start the ANT+ data source
 */
static bool ant_source_start(void) {
    if (m_device_id == 0) {
        NRF_LOG_WARNING("🚫 No ANT+ Device ID defined. Skipping ANT+ activation.");
        return false;
    }

    uint32_t err_code;

    NRF_LOG_INFO("🔄 Initializing ANT+ BPWR Channel...");

    // Set the ANT+ network key
    err_code = sd_ant_network_address_set(ANTPLUS_NETWORK_NUMBER, ANT_PLUS_NETWORK_KEY);
    if (err_code != NRF_SUCCESS) {
        NRF_LOG_ERROR("🚨 Failed to set ANT+ network key! Error: 0x%08X", err_code);
        return false;
    }
    NRF_LOG_INFO("✅ ANT+ Network Key Set Successfully!");

    // Configure the ANT+ channel
    static ant_channel_config_t bpwr_channel_config = {
        .channel_number    = ANT_BPWR_ANT_CHANNEL,
        .channel_type      = BPWR_DISP_CHANNEL_TYPE,  // ANT+ Receiver
        .ext_assign        = BPWR_EXT_ASSIGN,
        .rf_freq           = BPWR_ANTPLUS_RF_FREQ,   // Default ANT+ Frequency
        .transmission_type = ANT_BPWR_TRANS_TYPE,  
        .device_type       = 11, // ANT+ Bike Power
        .device_number     = 0,  // Placeholder, updated with m_device_id
        .channel_period    = BPWR_MSG_PERIOD,
        .network_number    = ANTPLUS_NETWORK_NUMBER,
    };

    // Set the device number dynamically
    bpwr_channel_config.device_number = m_device_id;
    NRF_LOG_INFO("Setting ANT+ Device ID to %d", m_device_id);

    // Initialize the ANT BPWR channel
    NRF_LOG_INFO("📡 Calling ant_bpwr_disp_init...");
    err_code = ant_bpwr_disp_init(&m_ant_bpwr, &bpwr_channel_config, &m_ant_bpwr_profile_bpwr_disp_config);
    if (err_code != NRF_SUCCESS) {
        NRF_LOG_ERROR("🚨 ant_bpwr_disp_init FAILED: 0x%08X", err_code);
        return false;
    }
    NRF_LOG_INFO("✅ ant_bpwr_disp_init SUCCESS!");

    // Open the ANT+ BPWR channel
    NRF_LOG_INFO("📡 Calling ant_bpwr_disp_open...");
    err_code = ant_bpwr_disp_open(&m_ant_bpwr);
    if (err_code != NRF_SUCCESS) {
        NRF_LOG_ERROR("🚨 ant_bpwr_disp_open FAILED: 0x%08X", err_code);
        return false;
    }
    
    m_ant_active = true;
    NRF_LOG_INFO("✅ ant_bpwr_disp_open SUCCESS!");
    
    return true;
}

/**
 * @brief Stop the ANT+ data source
 */
static void ant_source_stop(void) {
    NRF_LOG_INFO("🛑 Closing ANT+ Channel...");
    uint32_t err_code = sd_ant_channel_close(ANT_BPWR_ANT_CHANNEL);
    if (err_code == NRF_SUCCESS) {
        NRF_LOG_INFO("✅ ANT+ Channel Closed Successfully");
    } else {
        NRF_LOG_WARNING("⚠️ ANT+ Channel was already closed or error.");
    }
    
    m_ant_active = false;
}

/**
 * @brief Check if the ANT+ data source is active
 */
static bool ant_source_is_active(void) {
    return m_ant_active;
}

// Define the ANT+ data source interface
static const data_source_interface_t ant_source_interface = {
    .init = ant_source_init,
    .start = ant_source_start,
    .stop = ant_source_stop,
    .is_active = ant_source_is_active
};

// Public function to get the ANT+ data source interface
const data_source_interface_t* ant_data_source_get_interface(void) {
    return &ant_source_interface;
} 