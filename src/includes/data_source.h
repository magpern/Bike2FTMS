/**
 * @file data_source.h
 * @brief Data Source Interface 
 *
 * This file provides the interface that all data sources must implement.
 * A data source represents any component that can collect cycling data 
 * (power, cadence, etc.) from external devices.
 */

#ifndef DATA_SOURCE_H
#define DATA_SOURCE_H

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Data source types supported by the application
 */
typedef enum {
    DATA_SOURCE_NONE = 0,  /**< No data source */
    DATA_SOURCE_ANT_PLUS,  /**< ANT+ data source */
    DATA_SOURCE_BLE_PROPRIETARY  /**< Proprietary BLE data source */
} data_source_type_t;

/**
 * @brief Function pointer for data update callback
 * 
 * @param power_watts Power in watts
 * @param cadence_rpm Cadence in RPM
 */
typedef void (*data_update_callback_t)(uint16_t power_watts, uint8_t cadence_rpm);

/**
 * @brief Data source configuration
 */
typedef struct {
    data_source_type_t type;  /**< Type of data source */
    uint16_t device_id;       /**< Device ID for the source */
    data_update_callback_t data_callback;  /**< Callback for data updates */
} data_source_config_t;

/**
 * @brief Data source interface
 */
typedef struct {
    /**
     * @brief Initialize the data source
     * 
     * @param config Configuration for the data source
     * @return true if initialization was successful, false otherwise
     */
    bool (*init)(data_source_config_t* config);
    
    /**
     * @brief Start the data source
     * 
     * @return true if start was successful, false otherwise
     */
    bool (*start)(void);
    
    /**
     * @brief Stop the data source
     */
    void (*stop)(void);
    
    /**
     * @brief Check if the data source is active
     * 
     * @return true if active, false otherwise
     */
    bool (*is_active)(void);
} data_source_interface_t;

#endif /* DATA_SOURCE_H */ 