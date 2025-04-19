/**
 * @file cycling_data_model.c
 * @brief Implementation of the Cycling Data Model
 */

#include "includes/cycling_data_model.h"
#include <string.h>
#include "nrf_log.h"

// Moving average buffer size
#define MOVING_AVG_SIZE 6

// Callback for data updates
static void (*data_update_callback)(cycling_data_t data) = NULL;

// Data model
static cycling_data_t cycling_data;

// Moving average buffers
static uint16_t power_buffer[MOVING_AVG_SIZE] = {0};
static uint8_t cadence_buffer[MOVING_AVG_SIZE] = {0};
static uint8_t buffer_index = 0;
static bool buffer_filled = false;

/**
 * @brief Calculate moving average for power
 * 
 * @return uint16_t The moving average power
 */
static uint16_t calculate_moving_avg_power(void) {
    uint32_t sum = 0;
    for (uint8_t i = 0; i < MOVING_AVG_SIZE; i++) {
        sum += power_buffer[i];
    }
    return (uint16_t)(sum / MOVING_AVG_SIZE);
}

/**
 * @brief Calculate moving average for cadence
 * 
 * @return uint8_t The moving average cadence
 */
static uint8_t calculate_moving_avg_cadence(void) {
    uint32_t sum = 0;
    for (uint8_t i = 0; i < MOVING_AVG_SIZE; i++) {
        sum += cadence_buffer[i];
    }
    return (uint8_t)(sum / MOVING_AVG_SIZE);
}

bool cycling_data_init(void) {
    // Initialize the data model
    memset(&cycling_data, 0, sizeof(cycling_data_t));
    cycling_data.data_available = false;
    
    // Initialize buffers
    memset(power_buffer, 0, sizeof(power_buffer));
    memset(cadence_buffer, 0, sizeof(cadence_buffer));
    buffer_index = 0;
    buffer_filled = false;
    
    NRF_LOG_INFO("Cycling Data Model: Initialized");
    
    return true;
}

void cycling_data_update(uint16_t power_watts, uint8_t cadence_rpm) {
    // Store raw values
    cycling_data.instantaneous_power = power_watts;
    cycling_data.instantaneous_cadence = cadence_rpm;
    
    // Store in moving average buffers
    power_buffer[buffer_index] = power_watts;
    cadence_buffer[buffer_index] = cadence_rpm;
    
    // Update buffer index and filled flag
    buffer_index = (buffer_index + 1) % MOVING_AVG_SIZE;
    if (buffer_index == 0) {
        buffer_filled = true;
    }
    
    // Calculate and update averages
    if (buffer_filled) {
        cycling_data.average_power = calculate_moving_avg_power();
        cycling_data.average_cadence = calculate_moving_avg_cadence();
    } else {
        // Use instantaneous values until buffer is filled
        cycling_data.average_power = power_watts;
        cycling_data.average_cadence = cadence_rpm;
    }
    
    // Mark data as available
    cycling_data.data_available = true;
    
    NRF_LOG_DEBUG("Cycling Data: Power=%d W (avg=%d W), Cadence=%d RPM (avg=%d RPM)", 
                 cycling_data.instantaneous_power, 
                 cycling_data.average_power,
                 cycling_data.instantaneous_cadence,
                 cycling_data.average_cadence);
    
    // Notify callback if registered
    if (data_update_callback != NULL) {
        data_update_callback(cycling_data);
    }
}

cycling_data_t cycling_data_get(void) {
    return cycling_data;
}

void cycling_data_reset(void) {
    memset(&cycling_data, 0, sizeof(cycling_data_t));
    cycling_data.data_available = false;
    
    memset(power_buffer, 0, sizeof(power_buffer));
    memset(cadence_buffer, 0, sizeof(cadence_buffer));
    buffer_index = 0;
    buffer_filled = false;
    
    NRF_LOG_INFO("Cycling Data Model: Reset");
}

void cycling_data_register_callback(void (*callback)(cycling_data_t data)) {
    data_update_callback = callback;
    NRF_LOG_INFO("Cycling Data Model: Callback registered");
} 