/**
 * @file cycling_data_model.h
 * @brief Cycling Data Model 
 *
 * This file defines the data model for cycling metrics collected from
 * various data sources (ANT+, Proprietary BLE).
 */

#ifndef CYCLING_DATA_MODEL_H
#define CYCLING_DATA_MODEL_H

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Model for cycling data
 */
typedef struct {
    uint16_t instantaneous_power;   /**< Instantaneous power in watts */
    uint16_t average_power;         /**< Average power in watts */
    uint8_t instantaneous_cadence;  /**< Instantaneous cadence in RPM */
    uint8_t average_cadence;        /**< Average cadence in RPM */
    bool data_available;            /**< Indicates if valid data is available */
} cycling_data_t;

/**
 * @brief Initialize the cycling data model
 * 
 * @return true if initialization was successful, false otherwise
 */
bool cycling_data_init(void);

/**
 * @brief Update cycling data with new values
 * 
 * @param power_watts Power in watts
 * @param cadence_rpm Cadence in RPM
 */
void cycling_data_update(uint16_t power_watts, uint8_t cadence_rpm);

/**
 * @brief Get the current cycling data
 * 
 * @return cycling_data_t The current cycling data
 */
cycling_data_t cycling_data_get(void);

/**
 * @brief Reset the cycling data model
 */
void cycling_data_reset(void);

/**
 * @brief Register for cycling data updates
 * 
 * @param callback Function to call when cycling data is updated
 */
void cycling_data_register_callback(void (*callback)(cycling_data_t data));

#endif /* CYCLING_DATA_MODEL_H */ 