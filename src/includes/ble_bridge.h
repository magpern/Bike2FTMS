/**
 * @file ble_bridge.h
 * @brief BLE Bridge Interface 
 *
 * This file provides the interface for the BLE bridge that connects
 * the data model to the BLE services.
 */

#ifndef BLE_BRIDGE_H
#define BLE_BRIDGE_H

#include <stdint.h>
#include <stdbool.h>
#include "cycling_data_model.h"

/**
 * @brief Initialize the BLE bridge
 * 
 * @return true if initialization was successful, false otherwise
 */
bool ble_bridge_init(void);

/**
 * @brief Start the BLE bridge
 * 
 * @return true if successful, false otherwise
 */
bool ble_bridge_start(void);

/**
 * @brief Stop the BLE bridge
 */
void ble_bridge_stop(void);

/**
 * @brief Check if the BLE bridge is active
 * 
 * @return true if active, false otherwise
 */
bool ble_bridge_is_active(void);

/**
 * @brief Update the BLE services with new data
 * 
 * This function should be called when new data is available from the data model.
 * 
 * @param data The new cycling data
 */
void ble_bridge_update_data(cycling_data_t data);

/**
 * @brief Callback function for BLE events
 * 
 * This function should be called when BLE events occur (connect, disconnect, etc.)
 * 
 * @param connected true if connected, false if disconnected
 */
void ble_bridge_connection_event(bool connected);

#endif /* BLE_BRIDGE_H */ 