/**
 * Copyright (c) 2013 - 2021, Nordic Semiconductor ASA
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form, except as embedded into a Nordic
 *    Semiconductor ASA integrated circuit in a product or a software update for
 *    such product, must reproduce the above copyright notice, this list of
 *    conditions and the following disclaimer in the documentation and/or other
 *    materials provided with the distribution.
 *
 * 3. Neither the name of Nordic Semiconductor ASA nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * 4. This software, with or without modification, must only be used with a
 *    Nordic Semiconductor ASA integrated circuit.
 *
 * 5. Any software provided in binary form under this license must not be reverse
 *    engineered, decompiled, modified and/or disassembled.
 *
 * THIS SOFTWARE IS PROVIDED BY NORDIC SEMICONDUCTOR ASA "AS IS" AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY, NONINFRINGEMENT, AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL NORDIC SEMICONDUCTOR ASA OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */
/** @file
 *
 * @defgroup ble_sdk_app_ant_hrs_main main.c
 * @{
 * @ingroup ble_sdk_app_ant_hrs
 * @brief Refactored HRM sample application using both BLE and ANT.
 */

#include <stdint.h>
#include <string.h>
#include "common_definitions.h"

#include "nordic_common.h"
#include "app_error.h"
#include "app_timer.h"
#include "bsp.h"
#include "fds.h"
#include "nrf_sdh_ant.h"
#include "nrf_sdh_soc.h"
#include "nrf_pwr_mgmt.h"

#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
#include "app_timer.h"
#include "nrf_gpio.h"
#include "reed_sensor.h"
#include "ble_setup.h"
#include "ble_custom_config.h"
#include "nfc_handler.h"
#include "boards.h"
#include "nrf_delay.h"

// New includes for the refactored architecture
#include "includes/data_manager.h"
#include "includes/ble_bridge.h"
#include "includes/cycling_data_model.h"
#include "includes/data_source.h"

// Shutdown timer
APP_TIMER_DEF(m_shutdown_timer);
APP_TIMER_DEF(m_ble_delay_timer);

/**@brief Callback function for asserts in the SoftDevice.
 *
 * @details This function will be called in case of an assert in the SoftDevice.
 *
 * @warning This handler is an example only and does not fit a final product.
 * @warning On assert from the SoftDevice, the system can only recover on reset.
 *
 * @param[in]   line_num   Line number of the failing ASSERT call.
 * @param[in]   file_name  File name of the failing ASSERT call.
 */
void assert_nrf_callback(uint16_t line_num, const uint8_t * p_file_name)
{
    app_error_handler(DEAD_BEEF, line_num, p_file_name);
}

/**
 * @brief Function to enter system deep sleep mode
 * 
 * @details Performs all necessary steps to prepare the system for deep sleep
 */
void enter_deep_sleep(void)
{
    // Turn off all LEDs before sleep
    for (int i = 0; i < LEDS_NUMBER; ++i) {
        bsp_board_led_off(i);
    }

    // Enable reed sensor for wake
    reed_sensor_enable();

    // Enter Deep Sleep
    NRF_LOG_INFO("🛑 System entering deep sleep. Waiting for flywheel movement...");
    nrf_gpio_cfg_sense_input(REED_SWITCH_PIN, NRF_GPIO_PIN_PULLUP, NRF_GPIO_PIN_SENSE_LOW);
    sd_power_system_off();
}

/**
 * @brief Timer handler for delayed shutdown
 */
static void shutdown_timer_handler(void * p_context)
{
    // If no BLE connection after timeout, shut down
    if (m_conn_handle == BLE_CONN_HANDLE_INVALID) 
    {
        NRF_LOG_INFO("No BLE connection after timeout. Shutting down...");
        
        // Stop all components
        ble_bridge_stop();
        data_manager_stop_collection();

        enter_deep_sleep();
    }
    else
    {
        NRF_LOG_INFO("BLE is still connected; delaying shutdown...");
    }
}

/**
 * @brief Timer initialization.
 */
static void timers_init(void)
{
    // Initialize timer module
    uint32_t err_code = app_timer_init();
    APP_ERROR_CHECK(err_code);

    // Create shutdown timer
    err_code = app_timer_create(&m_shutdown_timer, APP_TIMER_MODE_SINGLE_SHOT, shutdown_timer_handler);
    APP_ERROR_CHECK(err_code);

    // Create BLE delay timer
    err_code = app_timer_create(&m_ble_delay_timer, APP_TIMER_MODE_REPEATED, shutdown_timer_handler);
    APP_ERROR_CHECK(err_code);
}

#define DOT_DURATION     200
#define DASH_DURATION    (DOT_DURATION * 3)
#define SYMBOL_PAUSE     DOT_DURATION

void morse_dot(void) {
    bsp_board_led_on(BSP_BOARD_LED_0);
    nrf_delay_ms(DOT_DURATION);
    bsp_board_led_off(BSP_BOARD_LED_0);
    nrf_delay_ms(SYMBOL_PAUSE);
}

void morse_dash(void) {
    bsp_board_led_on(BSP_BOARD_LED_0);
    nrf_delay_ms(DASH_DURATION);
    bsp_board_led_off(BSP_BOARD_LED_0);
    nrf_delay_ms(SYMBOL_PAUSE);
}

void morse_r(void) {
    morse_dot();    // .
    morse_dash();   // -
    morse_dot();    // .
}

// Callback for cycling data updates
static void cycling_data_callback(cycling_data_t data)
{
    // Forward data to BLE bridge
    ble_bridge_update_data(data);
}

/**@brief Application main function.
 */
int main(void)
{
    bsp_board_init(BSP_INIT_LEDS);
    bsp_board_leds_off();  // Turn off all LEDs
    morse_r();  // Flash ".-." (R) on LED_1

    uint32_t err_code;

    err_code = NRF_LOG_INIT(NULL);
    APP_ERROR_CHECK(err_code);
    NRF_LOG_DEFAULT_BACKENDS_INIT();

    timers_init();
    err_code = nrf_pwr_mgmt_init();
    APP_ERROR_CHECK(err_code);

    softdevice_setup();  // Initializes BLE and ANT+ stacks

    // Initialize FDS first
    custom_service_init();  // Initialize FDS

    reed_sensor_init(NULL);

    // Initialize BLE stack components
    gatt_init();
    gap_params_init();
    advertising_init();
    services_init();
    conn_params_init();

#ifdef BONDING_ENABLE
    bool erase_bonds = bsp_button_is_pressed(BOND_DELETE_ALL_BUTTON_ID);
    peer_manager_init(erase_bonds);
    if (erase_bonds) {
        NRF_LOG_INFO("Bonds erased!");
    }
#endif // BONDING_ENABLE


#if NRFX_NFCT_ENABLED  // Only initialize NFC if enabled
    // Initialize NFC
    nfc_init();
#endif

    NRF_LOG_INFO("🏁 Starting application with new architecture...");
    
    // Initialize data manager
    if (!data_manager_init()) {
        NRF_LOG_ERROR("Failed to initialize data manager");
        return -1;
    }
    
    // Initialize BLE bridge
    if (!ble_bridge_init()) {
        NRF_LOG_ERROR("Failed to initialize BLE bridge");
        return -1;
    }
    
    // Register callback for cycling data updates
    cycling_data_register_callback(cycling_data_callback);
    
    // Get device ID from settings
    uint16_t device_id = m_ant_device_id;
        
    // Set the data source based on configuration
    if (device_id == 0) {
        NRF_LOG_WARNING("🔄 Setup Mode: Data collection disabled, BLE Always On.");
        
        // Start BLE bridge only
        ble_bridge_start();
    } else {
        // Set data source based on configuration
        if (m_data_source_type == DATA_SOURCE_KEISER_M3I) {
            NRF_LOG_INFO("🔧 Using Keiser M3i data source");
            NRF_LOG_INFO("📡 MAC Address: %02X:%02X:%02X:%02X:%02X:%02X",
                        m_keiser_mac[0], m_keiser_mac[1], m_keiser_mac[2],
                        m_keiser_mac[3], m_keiser_mac[4], m_keiser_mac[5]);
            
            // Then set the data source
            if (!data_manager_set_data_source(DATA_SOURCE_KEISER_M3I, device_id)) {
                NRF_LOG_ERROR("Failed to set Keiser M3i data source");
                return -1;
            }
            
            // Finally start data collection
            if (!data_manager_start_collection()) {
                NRF_LOG_ERROR("Failed to start data collection");
                return -1;
            }
            
            // Start BLE bridge
            ble_bridge_start();
        } else {
            NRF_LOG_INFO("🔧 Using ANT+ data source");
            
            // Set ANT+ as the data source
            if (!data_manager_set_data_source(DATA_SOURCE_ANT_PLUS, device_id)) {
                NRF_LOG_ERROR("Failed to set ANT+ data source");
            } else {
                // Start data collection
                if (!data_manager_start_collection()) {
                    NRF_LOG_ERROR("Failed to start data collection");
                }
                
                // Start BLE bridge
                ble_bridge_start();
            }
        }
    }

    for (;;)
    {
        if (NRF_LOG_PROCESS() == false)
        {
            nrf_pwr_mgmt_run();
        }
    }
}

/**
 * @}
 */ 