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
 * @brief HRM sample application using both BLE and ANT.
 *
 * The application uses the BLE Heart Rate Service (and also the Device Information
 * services), and the ANT HRM RX profile.
 *
 * It will open a receive channel which will connect to an ANT HRM TX profile device when the
 * application starts. The received data will be propagated to a BLE central through the
 * BLE Heart Rate Service.
 *
 * The ANT HRM TX profile device simulator SDK application
 * (Board\pca10003\ant\ant_hrm\hrm_tx_buttons) can be used as a peer ANT device. By changing
 * ANT_HRMRX_NETWORK_KEY to the ANT+ Network Key, the application will instead be able to connect to
 * an ANT heart rate belt.
 *
 * @note The ANT+ Network Key is available for ANT+ Adopters. Please refer to
 *       http://thisisant.com to become an ANT+ Adopter and access the key.
 *
 * @note This application is based on the BLE Heart Rate Service Sample Application
 *       (Board\nrf6310\ble\ble_app_hrs). Please refer to this application for additional
 *       documentation.
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

#include "ant_error.h"
#include "ant_key_manager.h"
#include "ant_bpwr.h"
#include "ant_parameters.h"
#include "ant_interface.h"
#include "nfc_handler.h"

#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
#include "app_timer.h"
#include "nrf_gpio.h"
#include "reed_sensor.h"
#include "ble_setup.h"
#include "ble_battery_service.h"

static ant_bpwr_profile_t m_ant_bpwr; /* ANT Bike/Power profile instance */
// Forward declaration of ANT BPWR event handler
static void ant_bpwr_evt_handler(ant_bpwr_profile_t * p_profile, ant_bpwr_evt_t event);
// Register BLE & ANT event handlers
// Forward declarations

static void ant_evt_handler(ant_evt_t * p_ant_evt, void * p_context);


NRF_SDH_ANT_OBSERVER(m_ant_observer, APP_ANT_OBSERVER_PRIO, ant_evt_handler, NULL);
NRF_SDH_ANT_OBSERVER(m_ant_bpwr_observer, ANT_BPWR_ANT_OBSERVER_PRIO, ant_bpwr_disp_evt_handler, &m_ant_bpwr);
APP_TIMER_DEF(ant_restart_timer);  // Timer to restart ANT+

static bool system_sleep_pending = false;  // ✅ Track if sleep has already been triggered

static ant_bpwr_disp_cb_t m_ant_bpwr_disp_cb;  // Display callback structure

static const ant_bpwr_disp_config_t m_ant_bpwr_profile_bpwr_disp_config =
{
    .p_cb        = &m_ant_bpwr_disp_cb,
    .evt_handler = ant_bpwr_evt_handler,
};


/**@brief Callback function for asserts in the SoftDevice.
 *
 * @details This function will be called in case of an assert in the SoftDevice.
 *
 * @warning This handler is an example only and does not fit a final product. You need to analyze
 *          how your product is supposed to react in case of Assert.
 * @warning On assert from the SoftDevice, the system can only recover on reset.
 *
 * @param[in]   line_num   Line number of the failing ASSERT call.
 * @param[in]   file_name  File name of the failing ASSERT call.
 */
void assert_nrf_callback(uint16_t line_num, const uint8_t * p_file_name)
{
    app_error_handler(DEAD_BEEF, line_num, p_file_name);
}

static void ant_bpwr_rx_start(void)
{

    if (m_ant_device_id == 0)
    {
        NRF_LOG_WARNING("🚫 No ANT+ Device ID defined. Skipping ANT+ activation.");
        return;
    }

    uint32_t err_code;

    NRF_LOG_INFO("🔄 Initializing ANT+ BPWR Channel...");

    // 🔧 Set the ANT+ network key
    err_code = sd_ant_network_address_set(ANTPLUS_NETWORK_NUMBER, ANT_PLUS_NETWORK_KEY);
    if (err_code != NRF_SUCCESS)
    {
        NRF_LOG_ERROR("🚨 Failed to set ANT+ network key! Error: 0x%08X", err_code);
        return;
    }
    NRF_LOG_INFO("✅ ANT+ Network Key Set Successfully!");


    static ant_channel_config_t bpwr_channel_config =
    {
        .channel_number    = ANT_BPWR_ANT_CHANNEL,
        .channel_type      = BPWR_DISP_CHANNEL_TYPE,  // ANT+ Receiver
        .ext_assign        = BPWR_EXT_ASSIGN,
        .rf_freq           = BPWR_ANTPLUS_RF_FREQ,   // Default ANT+ Frequency
        .transmission_type = ANT_BPWR_TRANS_TYPE,  
        .device_type       = 11, // ANT+ Bike Power
        .device_number     = 0,  // Placeholder, will be updated later
        .channel_period    = BPWR_MSG_PERIOD,
        .network_number    = ANTPLUS_NETWORK_NUMBER,
    };

    // Print all parameters
    NRF_LOG_INFO("🔧 ANT+ Config:");
    NRF_LOG_INFO("   - Channel Number: %d", bpwr_channel_config.channel_number);
    NRF_LOG_INFO("   - Channel Type: %d", bpwr_channel_config.channel_type);
    NRF_LOG_INFO("   - RF Freq: %d", bpwr_channel_config.rf_freq);
    NRF_LOG_INFO("   - Device Type: %d", bpwr_channel_config.device_type);
    NRF_LOG_INFO("   - Channel Period: %d", bpwr_channel_config.channel_period);
    NRF_LOG_INFO("   - Network Number: %d", bpwr_channel_config.network_number);

    // ✅ Set the device number dynamically
    bpwr_channel_config.device_number = m_ant_device_id;
    NRF_LOG_INFO("Setting ANT+ Device ID to %d", m_ant_device_id);

    // Initialize the ANT BPWR channel
    NRF_LOG_INFO("📡 Calling ant_bpwr_disp_init...");
    err_code = ant_bpwr_disp_init(&m_ant_bpwr, &bpwr_channel_config, &m_ant_bpwr_profile_bpwr_disp_config);
    if (err_code != NRF_SUCCESS) {
        NRF_LOG_ERROR("🚨 ant_bpwr_disp_init FAILED: 0x%08X", err_code);
        return;
    }
    NRF_LOG_INFO("✅ ant_bpwr_disp_init SUCCESS!");

    
    // Open the ANT+ BPWR channel
    NRF_LOG_INFO("📡 Calling ant_bpwr_disp_open...");
    err_code = ant_bpwr_disp_open(&m_ant_bpwr);
    if (err_code != NRF_SUCCESS) {
        NRF_LOG_ERROR("🚨 ant_bpwr_disp_open FAILED: 0x%08X", err_code);
        return;
    }
    NRF_LOG_INFO("✅ ant_bpwr_disp_open SUCCESS!");
}

static void ant_restart_timer_handler(void *p_context)
{
    system_sleep_pending = false;  // ✅ Reset flag when waking up
    NRF_LOG_INFO("🔄 Timer expired - Restarting ANT+...");
    uint32_t err_code = ant_bpwr_disp_open(&m_ant_bpwr);
    if (err_code != NRF_SUCCESS) {
        NRF_LOG_ERROR("🚨 ant_bpwr_disp_open FAILED: 0x%08X", err_code);
        return;
    }
    ant_active = true;  // ✅ Set ANT+ active flag
    NRF_LOG_INFO("✅ ant_bpwr_disp_open SUCCESS!");
}

/**@brief Timer initialization.
 *
 * @details Initializes the timer module. This creates and starts application timers.
 */
static void timers_init(void)
{
    // Initialize timer module
    uint32_t err_code = app_timer_init();
    APP_ERROR_CHECK(err_code);

    NRF_LOG_INFO("🛠️ BLE send intervall timer...");
    // ✅ Create a repeating timer that triggers every 2 seconds
    err_code = app_timer_create(&m_ble_power_timer, APP_TIMER_MODE_REPEATED, ble_power_timer_handler);
    APP_ERROR_CHECK(err_code);

    NRF_LOG_INFO("🛠️ BLE shutdown timer...");

    // Initialize BLE shutdown timer
    err_code = app_timer_create(&ble_shutdown_timer, APP_TIMER_MODE_SINGLE_SHOT, ble_shutdown_timer_handler);
    APP_ERROR_CHECK(err_code);

    // ✅ Create the ANT+ restart timer (single-shot)
    err_code = app_timer_create(&ant_restart_timer, APP_TIMER_MODE_SINGLE_SHOT, ant_restart_timer_handler);
    APP_ERROR_CHECK(err_code);

}





void ant_evt_handler(ant_evt_t * p_ant_evt, void * p_context)
{
    #ifdef DEBUG  // ✅ Only flash LED in debug mode
    nrf_gpio_pin_toggle(LED4_PIN);       // Toggle LED2
    #endif
    switch (p_ant_evt->event)
    {
        case EVENT_RX:

            if (!ble_started)
            {
                NRF_LOG_INFO("✅ ANT+ Device Found! Starting BLE...");
                start_ble_advertising();
                ble_started = true;
                // ✅ Restart BLE Power Transmission Timer
                NRF_LOG_INFO("⏳ Starting BLE power transmission timer...");
                app_timer_start(m_ble_power_timer, APP_TIMER_TICKS(2000), NULL);
            }
            // Cancel shutdown if BLE is running and ANT+ is active
            ant_active = true;
            app_timer_stop(ble_shutdown_timer);

            // Process the received data
            uint8_t * p_page_buffer = p_ant_evt->message.ANT_MESSAGE_aucPayload;
            uint8_t page_number = p_page_buffer[0];  // Page number is always in Byte 0
    
            if (page_number == 0x52)  // 🔋 Battery Status (Common Page 82)
            {
                NRF_LOG_INFO("📡 ANT+ Message Received - Page: %d", page_number);

                uint8_t battery_id = (p_page_buffer[1] & 0xF0) >> 4;  // Extract battery ID (upper 4 bits)
                uint8_t num_batteries = p_page_buffer[1] & 0x0F;  // Extract number of batteries (lower 4 bits)
    
                uint32_t operating_time = (p_page_buffer[2]) |
                                          (p_page_buffer[3] << 8) |
                                          (p_page_buffer[4] << 16);
    
                uint8_t battery_voltage_frac = p_page_buffer[5];  // Fractional battery voltage
                float battery_voltage = battery_voltage_frac / 256.0;  // Convert to volts
    
                uint8_t status_flags = p_page_buffer[6];  // Battery status information
                
                NRF_LOG_INFO("🔋 Battery Status:");
                NRF_LOG_INFO("   - Battery ID: %d", battery_id);
                NRF_LOG_INFO("   - Number of Batteries: %d", num_batteries);
                NRF_LOG_INFO("   - Operating Time: %d seconds", operating_time * 2);  // Convert to actual seconds
                NRF_LOG_INFO("   - Battery Voltage: %.2fV", battery_voltage);
                NRF_LOG_INFO("   - Status Flags: 0x%02X", status_flags);
                
                // Decode battery status flags
                if (status_flags & 0x01) {
                    NRF_LOG_INFO("   - ⚠️ Battery Low");
                }
                if (status_flags & 0x02) {
                    NRF_LOG_INFO("   - 🔄 Battery Replaced");
                }
            }
            break;

        case EVENT_RX_SEARCH_TIMEOUT:
        case EVENT_CHANNEL_CLOSED:
            if (!system_sleep_pending)
            {
                NRF_LOG_WARNING("⚠️ ANT+ Connection Lost. Stopping BLE and entering deep sleep...");
                ant_active = false;
                system_sleep_pending = true;
        
                // ✅ Stop BLE Immediately
                stop_ble_advertising();
                ble_started = false;

                // ✅ Try to Close ANT+ Channel
                NRF_LOG_INFO("🛑 Closing ANT+ Channel...");
                uint32_t err_code = sd_ant_channel_close(ANT_BPWR_ANT_CHANNEL);
                if (err_code == NRF_SUCCESS) {
                    NRF_LOG_INFO("✅ ANT+ Channel Closed Successfully");
                } else {
                    NRF_LOG_WARNING("⚠️ ANT+ Channel was already closed.");
                }
        
                // ✅ Enable reed switch before deep sleep
                reed_sensor_enable();
        
                // ✅ Enter Deep Sleep
                NRF_LOG_INFO("🛑 System entering deep sleep. Waiting for flywheel movement...");
                nrf_gpio_cfg_sense_input(REED_SWITCH_PIN, NRF_GPIO_PIN_PULLUP, NRF_GPIO_PIN_SENSE_LOW);
                sd_power_system_off();
            }
            break;
        
            
            
        case EVENT_RX_FAIL:
            NRF_LOG_WARNING("⚠️ ANT+ RX Fail: %d", p_ant_evt->message.ANT_MESSAGE_ucMesgID);
            break;
        case EVENT_RX_DATA_OVERFLOW:
            NRF_LOG_WARNING("⚠️ ANT+ RX Data Overflow: %d", p_ant_evt->message.ANT_MESSAGE_ucMesgID);
            break;
        default:
            NRF_LOG_INFO("⚠️ Unhandled ANT+ event: %d", p_ant_evt->event);
            break;
    }
}


/** @brief Handle received ANT+ BPWR data.
 *
 * @param[in]   p_profile       Pointer to the ANT+ BPWR profile instance.
 * @param[in]   event           Event related to ANT+ BPWR Display profile.
 */
static void ant_bpwr_evt_handler(ant_bpwr_profile_t * p_profile, ant_bpwr_evt_t event)
{
    if (p_profile == NULL)
    {
        NRF_LOG_ERROR("🚨 ERROR: p_profile is NULL!");
        return;
    }

    switch (event)
    {
        case ANT_BPWR_PAGE_16_UPDATED:
        {
            uint8_t event_count = p_profile->page_16.update_event_count;  // ✅ Extract counter
            NRF_LOG_INFO("🔄 ANT+ Power Event Count: %d", event_count);
            // ✅ Store latest power & cadence values (DO NOT send BLE yet)
            latest_power_watts = p_profile->page_16.instantaneous_power;
            latest_cadence_rpm = p_profile->common.instantaneous_cadence;

            NRF_LOG_INFO("🚴 Updated Power: %d W, Cadence: %d RPM (Stored)", latest_power_watts, latest_cadence_rpm);
            break;
        }

        case ANT_BPWR_PAGE_80_UPDATED:  // 📋 Manufacturer Info (Page 80)
        {
            uint16_t manufacturer_id = p_profile->page_80.manufacturer_id;
            uint16_t model_number = p_profile->page_80.model_number;
            uint8_t hardware_revision = p_profile->page_80.hw_revision;

            NRF_LOG_INFO("📋 Manufacturer Info - HW Rev: %d, Manufacturer: %d, Model: %d",
                         hardware_revision, manufacturer_id, model_number);
            break;
        }

        case ANT_BPWR_PAGE_81_UPDATED:  // 📋 Product Information (Page 81)
        {
            uint8_t sw_revision_main = p_profile->page_81.sw_revision_major;
            uint8_t sw_revision_supplemental = p_profile->page_81.sw_revision_minor;
            uint32_t serial_number = p_profile->page_81.serial_number;

            uint16_t software_version = (sw_revision_supplemental == 0xFF) ?
                                        sw_revision_main :
                                        (sw_revision_main * 100) + sw_revision_supplemental;

            NRF_LOG_INFO("📋 Product Info - SW Version: %d, Serial Number: %u",
                         software_version, serial_number);
            break;
        }

        default:
            NRF_LOG_WARNING("⚠️ Unknown ANT+ Page Update: %d", event);
            break;
    }
}



/**@brief Application main function.
 */
int main(void)
{

    #ifdef DEBUG  // ✅ Only flash LED in debug mode
    nrf_gpio_cfg_output(LED4_PIN);       // Set LED2 as output
    nrf_gpio_cfg_output(LED3_PIN);       // Set LED2 as output
    #endif

    uint32_t err_code;

    err_code = NRF_LOG_INIT(NULL);
    APP_ERROR_CHECK(err_code);
    NRF_LOG_DEFAULT_BACKENDS_INIT();

    timers_init();
    err_code = nrf_pwr_mgmt_init();
    APP_ERROR_CHECK(err_code);

    custom_service_load_from_flash();  // Load stored ANT ID & BLE Name

    softdevice_setup();  // Initializes BLE and ANT+ stacks

    reed_sensor_init(NULL);

    //bsp_init(BSP_INIT_LEDS | BSP_INIT_BUTTONS, NULL);
    
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


    #if NRFX_NFCT_ENABLED  // ✅ Only initialize NFC if enabled
    // Initialize NFC
        nfc_init();
    #endif

    NRF_LOG_INFO("🏁 Starting BLE and ANT+ independently...");
    
    start_ble_advertising();  // Start BLE advertising
        // ✅ Check if we need to activate ANT+
    if (m_ant_device_id == 0)
    {
        NRF_LOG_WARNING("🔄 Setup Mode: ANT+ Disabled, BLE Always On.");
    }
    else
    {
        ant_bpwr_rx_start();  // Start ANT+ reception
    }

    // ✅ Start BLE periodic update timer (only if ANT+ is active)
    if (m_ant_device_id != 0)
    {
        err_code = app_timer_start(m_ble_power_timer, APP_TIMER_TICKS(2000), NULL);
        APP_ERROR_CHECK(err_code);
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
