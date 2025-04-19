/**
 * @file data_manager.c
 * @brief Implementation of the Data Manager
 */

#include "includes/data_manager.h"
#include "includes/cycling_data_model.h"
#include "ant/ant_data_source.h"
#include "nrf_log.h"

// Forward declare the proprietary BLE source interface (will be implemented later)
extern const data_source_interface_t* prop_ble_data_source_get_interface(void);

// Currently active data source
static data_source_type_t m_active_source_type = DATA_SOURCE_NONE;
static const data_source_interface_t* m_active_source = NULL;
static uint16_t m_active_device_id = 0;

// Callback function for data source updates
static void data_source_callback(uint16_t power_watts, uint8_t cadence_rpm) {
    // Update the cycling data model with new values
    cycling_data_update(power_watts, cadence_rpm);
}

bool data_manager_init(void) {
    // Initialize the cycling data model
    if (!cycling_data_init()) {
        NRF_LOG_ERROR("Failed to initialize cycling data model");
        return false;
    }
    
    NRF_LOG_INFO("Data Manager: Initialized");
    return true;
}

bool data_manager_set_data_source(data_source_type_t type, uint16_t device_id) {
    // If already active, stop collection first
    if (m_active_source != NULL && m_active_source->is_active()) {
        m_active_source->stop();
    }
    
    // Store the new device ID
    m_active_device_id = device_id;
    
    // Set up the new data source
    switch (type) {
        case DATA_SOURCE_ANT_PLUS:
            m_active_source = ant_data_source_get_interface();
            m_active_source_type = DATA_SOURCE_ANT_PLUS;
            NRF_LOG_INFO("Data Manager: Set data source to ANT+");
            break;
            
        case DATA_SOURCE_BLE_PROPRIETARY:
            // Will be implemented later
            //m_active_source = prop_ble_data_source_get_interface();
            //m_active_source_type = DATA_SOURCE_BLE_PROPRIETARY;
            //NRF_LOG_INFO("Data Manager: Set data source to Proprietary BLE");
            
            // For now, return false since not implemented
            NRF_LOG_WARNING("Data Manager: Proprietary BLE source not implemented yet");
            return false;
            
        case DATA_SOURCE_NONE:
            m_active_source = NULL;
            m_active_source_type = DATA_SOURCE_NONE;
            NRF_LOG_INFO("Data Manager: Disabled data source");
            return true;
            
        default:
            NRF_LOG_ERROR("Data Manager: Unknown data source type %d", type);
            return false;
    }
    
    // Configure and initialize the data source
    if (m_active_source != NULL) {
        data_source_config_t config = {
            .type = type,
            .device_id = device_id,
            .data_callback = data_source_callback
        };
        
        if (!m_active_source->init(&config)) {
            NRF_LOG_ERROR("Data Manager: Failed to initialize data source");
            m_active_source = NULL;
            m_active_source_type = DATA_SOURCE_NONE;
            return false;
        }
    }
    
    return true;
}

data_source_type_t data_manager_get_active_source_type(void) {
    return m_active_source_type;
}

bool data_manager_start_collection(void) {
    if (m_active_source == NULL) {
        NRF_LOG_ERROR("Data Manager: No active data source to start");
        return false;
    }
    
    // Reset the cycling data model
    cycling_data_reset();
    
    // Start the data source
    if (!m_active_source->start()) {
        NRF_LOG_ERROR("Data Manager: Failed to start data source");
        return false;
    }
    
    NRF_LOG_INFO("Data Manager: Started data collection");
    return true;
}

void data_manager_stop_collection(void) {
    if (m_active_source != NULL && m_active_source->is_active()) {
        m_active_source->stop();
        NRF_LOG_INFO("Data Manager: Stopped data collection");
    }
}

bool data_manager_is_active(void) {
    return (m_active_source != NULL && m_active_source->is_active());
}

cycling_data_t data_manager_get_latest_data(void) {
    return cycling_data_get();
} 