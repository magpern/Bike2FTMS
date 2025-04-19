/**
 * @file data_manager.h
 * @brief Data Manager Interface 
 *
 * This file provides the interface for the data manager that coordinates
 * between data sources and BLE services.
 */

#ifndef DATA_MANAGER_H
#define DATA_MANAGER_H

#include <stdint.h>
#include <stdbool.h>
#include "data_source.h"
#include "cycling_data_model.h"

/**
 * @brief Initialize the data manager
 * 
 * @return true if initialization was successful, false otherwise
 */
bool data_manager_init(void);

/**
 * @brief Set the active data source type
 * 
 * @param type Type of data source to activate
 * @param device_id Device ID for the data source
 * @return true if successful, false otherwise
 */
bool data_manager_set_data_source(data_source_type_t type, uint16_t device_id);

/**
 * @brief Get the current active data source type
 * 
 * @return data_source_type_t Current active data source type
 */
data_source_type_t data_manager_get_active_source_type(void);

/**
 * @brief Start the data collection from the active source
 * 
 * @return true if successful, false otherwise
 */
bool data_manager_start_collection(void);

/**
 * @brief Stop the data collection from the active source
 */
void data_manager_stop_collection(void);

/**
 * @brief Check if data collection is active
 * 
 * @return true if active, false otherwise
 */
bool data_manager_is_active(void);

/**
 * @brief Get the latest cycling data
 * 
 * @return cycling_data_t The latest cycling data
 */
cycling_data_t data_manager_get_latest_data(void);

#endif /* DATA_MANAGER_H */ 