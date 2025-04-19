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

// BLE update timer
APP_TIMER_DEF(m_ble_update_timer);

// State tracking
static bool m_bridge_active = false;
static bool m_data_ready = false;
static cycling_data_t m_latest_data;

// Function to handle BLE timer expiration
static void ble_update_timer_handler(void * p_context) {
    if (!m_data_ready || !m_bridge_active) {
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

bool ble_bridge_init(void) {
    uint32_t err_code;
    
    // Create a timer for periodic BLE updates
    err_code = app_timer_create(&m_ble_update_timer, APP_TIMER_MODE_REPEATED, ble_update_timer_handler);
    APP_ERROR_CHECK(err_code);
    
    // Initialize state variables
    m_bridge_active = false;
    m_data_ready = false;
    
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
    
    m_bridge_active = true;
    
    NRF_LOG_INFO("BLE Bridge: Started");
    
    return true;
}

void ble_bridge_stop(void) {
    // Stop BLE advertising
    stop_ble_advertising();
    
    // Stop the BLE update timer
    app_timer_stop(m_ble_update_timer);
    
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
    
    // Log only in debug mode to avoid excessive logging
    NRF_LOG_DEBUG("BLE Bridge: Data updated - Power=%d W, Cadence=%d RPM", 
                  data.average_power, data.average_cadence);
}

void ble_bridge_connection_event(bool connected) {
    if (connected) {
        NRF_LOG_INFO("BLE Bridge: Device connected");
    } else {
        NRF_LOG_INFO("BLE Bridge: Device disconnected");
    }
} 