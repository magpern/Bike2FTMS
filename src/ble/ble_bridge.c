/**
 * @file ble_bridge.c
 * @brief Implementation of the BLE Bridge
 */

#include "includes/ble_bridge.h"
#include "ble/ble_setup.h"
#include "ble/ble_ftms.h"
#include "ble/ble_cps.h"
#include "common_definitions.h"
#include "nrf_log.h"
#include "app_timer.h"
#include "boards.h"

// Forward declaration of sleep function from main
extern void enter_deep_sleep(void);

// BLE update timer
APP_TIMER_DEF(m_ble_update_timer);
// Inactivity timer for power saving
APP_TIMER_DEF(m_inactivity_timer);

// Constants
#define INACTIVITY_TIMEOUT_MS  20000  // 20 seconds inactivity before sleep
#define INACTIVITY_CHECK_MS    2000   // Check inactivity every second
#define DATA_TIMEOUT_MS        3000   // 3 seconds without data before zeroing values
#define APP_TIMER_PRESCALER    0      // Timer prescaler value
#define APP_TIMER_CLOCK_FREQ   32768  // Timer clock frequency in Hz

// State tracking
static bool m_bridge_active = false;
static bool m_data_ready = false;
static bool m_is_connected = false;
static cycling_data_t m_latest_data;
static uint32_t m_last_data_timestamp = 0;
static uint32_t m_last_connection_timestamp = 0;

// Function to convert timer ticks to milliseconds
static uint32_t ticks_to_ms(uint32_t ticks) {
    return ((ticks * 1000) / (APP_TIMER_CLOCK_FREQ / (APP_TIMER_PRESCALER + 1)));
}

// Function to handle BLE timer expiration
static void ble_update_timer_handler(void * p_context) {
    if (!m_bridge_active) {
        return;
    }

    #if defined(DEBUG) && !defined(RELEASE)
        bsp_board_led_invert(1);       // Toggle LED2
    #endif
    
    uint32_t current_time = app_timer_cnt_get();
    uint32_t time_since_data = app_timer_cnt_diff_compute(current_time, m_last_data_timestamp);
    time_since_data = ticks_to_ms(time_since_data);
    
    // If we have no data or data is stale, send zero values
    if (!m_data_ready || time_since_data >= DATA_TIMEOUT_MS) {
        NRF_LOG_DEBUG("BLE Bridge: No recent data, sending zero values");
        
        // Update Cycling Power Service
        if (m_cps.conn_handle != BLE_CONN_HANDLE_INVALID) {
            ble_cps_send_power_measurement(&m_cps, 0);
        }

        // Update Fitness Machine Service
        if (m_ftms.conn_handle != BLE_CONN_HANDLE_INVALID) {
            ble_ftms_tick(&m_ftms, 0, 0);
        }
        return;
    }
    
    // Update the BLE services with the latest data
    NRF_LOG_DEBUG("BLE Bridge: Updating services with Power=%d W, Cadence=%d RPM", 
                  m_latest_data.average_power, m_latest_data.average_cadence);

    // Update Cycling Power Service
    if (m_cps.conn_handle != BLE_CONN_HANDLE_INVALID) {
        ble_cps_send_power_measurement(&m_cps, m_latest_data.average_power);
    }

    // Update Fitness Machine Service
    if (m_ftms.conn_handle != BLE_CONN_HANDLE_INVALID) {
        ble_ftms_tick(&m_ftms, m_latest_data.average_power, m_latest_data.average_cadence);
    }
}

// Function to check inactivity and enter deep sleep if needed
static void inactivity_timer_handler(void * p_context) {
    uint32_t current_time = app_timer_cnt_get();
    uint32_t time_since_data = app_timer_cnt_diff_compute(current_time, m_last_data_timestamp);
    uint32_t time_since_connection = app_timer_cnt_diff_compute(current_time, m_last_connection_timestamp);
    
    // Convert ticks to ms
    time_since_data = ticks_to_ms(time_since_data);
    time_since_connection = ticks_to_ms(time_since_connection);
    
    // If we're connected, reset the connection timestamp to now
    // This keeps "time since connection" close to zero while connected
    if (m_is_connected) {
        m_last_connection_timestamp = current_time;
        time_since_connection = 0;
    }
    
    NRF_LOG_INFO("BLE Bridge: Inactivity check - Connected: %d, Time since data: %d ms, Time since disconnect: %d ms", 
                  m_is_connected, time_since_data, time_since_connection);
    
    // If no connection for 20 seconds
    if (!m_is_connected && time_since_connection >= INACTIVITY_TIMEOUT_MS) {
        NRF_LOG_INFO("BLE Bridge: No connection for %d ms, entering deep sleep", time_since_connection);
        enter_deep_sleep();
        return;
    }
    
    // If we have a BLE connection
    if (m_is_connected) {
        NRF_LOG_INFO("BLE Bridge: Device is connected, staying active");
        // Only check data timeout if we have a data timestamp
        if (m_last_data_timestamp > 0) {
            if (time_since_data >= INACTIVITY_TIMEOUT_MS) {
                NRF_LOG_INFO("BLE Bridge: No data for %d ms, entering deep sleep", time_since_data);
                enter_deep_sleep();
                return;
            }
        } else {
            NRF_LOG_INFO("BLE Bridge: Connected but no data timestamp yet");
        }
    }
}

bool ble_bridge_init(void) {
    uint32_t err_code;
    
    // Create a timer for periodic BLE updates
    err_code = app_timer_create(&m_ble_update_timer, APP_TIMER_MODE_REPEATED, ble_update_timer_handler);
    APP_ERROR_CHECK(err_code);
    
    // Create a timer for inactivity checking
    err_code = app_timer_create(&m_inactivity_timer, APP_TIMER_MODE_REPEATED, inactivity_timer_handler);
    APP_ERROR_CHECK(err_code);
    
    // Initialize state variables
    m_bridge_active = false;
    m_data_ready = false;
    m_is_connected = false;
    m_last_data_timestamp = 0;
    m_last_connection_timestamp = app_timer_cnt_get(); // Start counting from init
    
    NRF_LOG_INFO("BLE Bridge: Initialized");
    
    return true;
}

bool ble_bridge_start(void) {
    uint32_t err_code;
    
    // Start BLE advertising
    start_ble_advertising();
    
    // Start the BLE update timer (every 1 second)
    err_code = app_timer_start(m_ble_update_timer, APP_TIMER_TICKS(1000), NULL);
    APP_ERROR_CHECK(err_code);
    
    // Start the inactivity timer
    err_code = app_timer_start(m_inactivity_timer, APP_TIMER_TICKS(INACTIVITY_CHECK_MS), NULL);
    APP_ERROR_CHECK(err_code);
    
    m_bridge_active = true;
    
    NRF_LOG_INFO("BLE Bridge: Started");
    
    return true;
}

void ble_bridge_stop(void) {
    // Stop BLE advertising
    stop_ble_advertising();
    
    // Stop the BLE update timer
    app_timer_stop(m_ble_update_timer);
    
    // Stop the inactivity timer
    app_timer_stop(m_inactivity_timer);
    
    m_bridge_active = false;
    
    NRF_LOG_INFO("BLE Bridge: Stopped");
}

bool ble_bridge_is_active(void) {
    return m_bridge_active;
}

void ble_bridge_update_data(cycling_data_t data) {
    // Store the latest data
    m_latest_data = data;
    m_data_ready = true;
    
    // Update the timestamp of the last data received
    m_last_data_timestamp = app_timer_cnt_get();
    
    // Log only in debug mode to avoid excessive logging
    NRF_LOG_DEBUG("BLE Bridge: Data updated - Power=%d W, Cadence=%d RPM", 
                  data.average_power, data.average_cadence);
}

void ble_bridge_connection_event(bool connected) {
    m_is_connected = connected;
    m_last_connection_timestamp = app_timer_cnt_get();
    
    if (connected) {
        NRF_LOG_INFO("BLE Bridge: Device connected");
        // Reset the data timestamp when we get a connection
        m_last_data_timestamp = app_timer_cnt_get();
    } else {
        NRF_LOG_INFO("BLE Bridge: Device disconnected");
    }
}

// Function to notify the bridge that ANT+ data source has been lost
void ble_bridge_data_source_lost(void) {
    NRF_LOG_INFO("BLE Bridge: Data source lost, entering deep sleep");
    enter_deep_sleep();
}

void ble_bridge_reset_data_timestamp(void) {
    m_last_data_timestamp = app_timer_cnt_get();
    NRF_LOG_DEBUG("BLE Bridge: Reset data timestamp");
} 