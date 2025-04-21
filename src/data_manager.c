/**
 * @file data_manager.c
 * @brief Implementation of the Data Manager
 */

#include "includes/data_manager.h"
#include "includes/cycling_data_model.h"
#include "ant/ant_data_source.h"
#include "includes/keiser_m3i_data_source.h"
#include "includes/ble_bridge.h"
#include "nrf_log.h"
#include "boards.h"
// Forward declare the proprietary BLE source interface (will be implemented later)
extern const data_source_interface_t* prop_ble_data_source_get_interface(void);

// Currently active data source
static data_source_type_t m_active_source_type = DATA_SOURCE_NONE;
static const data_source_interface_t* m_active_source = NULL;

// Callback function for data source updates
static void data_source_callback(uint16_t power, uint8_t cadence)
{
    NRF_LOG_DEBUG("Data Manager: Received data update - Power: %d W, Cadence: %d RPM", power, cadence);
    
    // Update cycling data model
    cycling_data_update(power, cadence);
    
    cycling_data_t current_data = cycling_data_get();
    NRF_LOG_DEBUG("Data Manager: Updated cycling data model - Power: %d W, Cadence: %d RPM", 
                 current_data.instantaneous_power, current_data.instantaneous_cadence);

    #if defined(DEBUG) && !defined(RELEASE)
        bsp_board_led_invert(3);       // Toggle LED2
    #endif

}

/**
 * @brief Get the interface for the specified data source type
 * 
 * @param type The type of data source to get
 * @return const data_source_interface_t* Pointer to the data source interface, or NULL if not found
 */
static const data_source_interface_t* data_source_get_interface(data_source_type_t type)
{
    switch (type)
    {
        case DATA_SOURCE_ANT_PLUS:
            return ant_data_source_get_interface();
            
        case DATA_SOURCE_KEISER_M3I:
            return keiser_m3i_data_source_get_interface();
            
        case DATA_SOURCE_BLE_PROPRIETARY:
            // Will be implemented later
            return NULL;
            
        case DATA_SOURCE_NONE:
            return NULL;
            
        default:
            NRF_LOG_ERROR("Data Manager: Unknown data source type: %d", type);
            return NULL;
    }
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

bool data_manager_set_data_source(data_source_type_t type, uint16_t device_id)
{
    NRF_LOG_INFO("Data Manager: Setting data source type: %d", type);
    
    // Stop current data source if active
    if (m_active_source != NULL)
    {
        NRF_LOG_INFO("Data Manager: Stopping current data source");
        m_active_source->stop();
        m_active_source = NULL;
    }

    // Get new data source interface
    m_active_source = data_source_get_interface(type);
    if (m_active_source == NULL)
    {
        NRF_LOG_ERROR("Data Manager: Failed to get data source interface for type: %d", type);
        return false;
    }

    // Initialize the data source
    data_source_config_t config = {
        .type = type,
        .device_id = device_id,
        .data_callback = data_source_callback
    };

    // Initialize data source
    if (!m_active_source->init(&config))
    {
        NRF_LOG_ERROR("Data Manager: Failed to initialize data source");
        m_active_source = NULL;
        return false;
    }

    // Start data source
    if (!m_active_source->start())
    {
        NRF_LOG_ERROR("Data Manager: Failed to start data source");
        m_active_source = NULL;
        return false;
    }

    NRF_LOG_INFO("Data Manager: Successfully set and started data source");
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