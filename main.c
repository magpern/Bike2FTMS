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
#include "nordic_common.h"
#include "app_error.h"
#include "app_timer.h"
#include "ble.h"
#include "ble_hci.h"
#include "ble_conn_state.h"
#include "ble_conn_params.h"
#include "ble_srv_common.h"
#include "ble_advdata.h"
#include "ble_dis.h"
#include "bsp.h"
#include "fds.h"
#include "peer_manager.h"
#include "peer_manager_handler.h"
#include "nrf_ble_gatt.h"
#include "nrf_sdh.h"
#include "nrf_sdh_ble.h"
#include "nrf_sdh_ant.h"
#include "nrf_sdh_soc.h"
#include "nrf_ble_qwr.h"
#include "nrf_pwr_mgmt.h"


#include "ant_error.h"
#include "ant_key_manager.h"
//#include "ant_hrm.h"
#include "ant_bpwr.h"
#include "ant_parameters.h"
#include "ant_interface.h"

#include "ble_ftms.h"


#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
#include "app_timer.h"

#define WAKEUP_BUTTON_ID                0                                            /**< Button used to wake up the application. */
#define BOND_DELETE_ALL_BUTTON_ID       1                                            /**< Button used for deleting all bonded centrals during startup. */

#define DEVICE_NAME                     "Nordic_HRM"                                 /**< Name of device. Will be included in the advertising data. */
#define MANUFACTURER_NAME               "NordicSemiconductor"                        /**< Manufacturer. Will be passed to Device Information Service. */
#define APP_ADV_INTERVAL                40                                           /**< The advertising interval (in units of 0.625 ms. This value corresponds to 25 ms). */
#define APP_ADV_DURATION                18000                                        /**< The advertising duration in units of seconds. */

#define APP_BLE_CONN_CFG_TAG            1                                            /**< A tag that refers to the BLE stack configuration we set with @ref sd_ble_cfg_set. Default tag is @ref BLE_CONN_CFG_TAG_DEFAULT. */

#define IS_SRVC_CHANGED_CHARACT_PRESENT 0                                            /**< Whether or not to include the service_changed characteristic. If not enabled, the server's database cannot be changed for the lifetime of the device */

#define CONN_INTERVAL_BASE              80                                           /**< Definition of 100 ms, when 1 unit is 1.25 ms. */
#define SECOND_10_MS_UNITS              100                                          /**< Definition of 1 second, when 1 unit is 10 ms. */
#define MIN_CONN_INTERVAL               (CONN_INTERVAL_BASE / 2)                     /**< Minimum acceptable connection interval (50 ms), Connection interval uses 1.25 ms units. */
#define MAX_CONN_INTERVAL               (CONN_INTERVAL_BASE)                         /**< Maximum acceptable connection interval (100 ms), Connection interval uses 1.25 ms units. */
#define SLAVE_LATENCY                   0                                            /**< Slave latency. */
#define CONN_SUP_TIMEOUT                (4 * SECOND_10_MS_UNITS)                     /**< Connection supervisory timeout (4 seconds), Supervision Timeout uses 10 ms units. */

#define FIRST_CONN_PARAMS_UPDATE_DELAY  APP_TIMER_TICKS(5000)                        /**< Time from initiating event (connect or start of notification) to first time sd_ble_gap_conn_param_update is called (5 seconds). */
#define NEXT_CONN_PARAMS_UPDATE_DELAY   APP_TIMER_TICKS(30000)                       /**< Time between each call to sd_ble_gap_conn_param_update after the first call (30 seconds). */
#define MAX_CONN_PARAMS_UPDATE_COUNT    3                                            /**< Number of attempts before giving up the connection parameter negotiation. */

#define SEC_PARAM_TIMEOUT               30                                           /**< Timeout for Pairing Request or Security Request (in seconds). */
#define SEC_PARAM_BOND                  1                                            /**< Perform bonding. */
#define SEC_PARAM_MITM                  0                                            /**< Man In The Middle protection not required. */
#define SEC_PARAM_IO_CAPABILITIES       BLE_GAP_IO_CAPS_NONE                         /**< No I/O capabilities. */
#define SEC_PARAM_OOB                   0                                            /**< Out Of Band data not available. */
#define SEC_PARAM_MIN_KEY_SIZE          7                                            /**< Minimum encryption key size. */
#define SEC_PARAM_MAX_KEY_SIZE          16                                           /**< Maximum encryption key size. */

#define DEAD_BEEF                       0xDEADBEEF                                   /**< Value used as error code on stack dump, can be used to identify stack location on stack unwind. */

#define ANT_BPWR_ANT_CHANNEL           0                                            /**< Default ANT Channel. */
#define ANT_BPWR_DEVICE_NUMBER         18465                                            /**< Device Number. */
#define ANT_BPWR_TRANS_TYPE            5                                            /**< Transmission Type. */
#define ANTPLUS_NETWORK_NUMBER          0                                            /**< Network number. */
#define ANT_PLUS_NETWORK_KEY ((uint8_t[8]){0xB9, 0xA5, 0x21, 0xFB, 0xBD, 0x72, 0xC3, 0x45})

static volatile uint16_t                m_conn_handle = BLE_CONN_HANDLE_INVALID;     /**< Handle of the current connection. */
static uint8_t                          m_adv_handle;                                /**< Advertising handle. */
NRF_BLE_QWR_DEF(m_qwr);                                                              /**< Context for the Queued Write module.*/
NRF_BLE_GATT_DEF(m_gatt);                                                            /**< GATT module instance. */

static ble_ftms_t m_ftms;  // BLE FTMS Service Instance


static ant_bpwr_profile_t m_ant_bpwr; /* ANT Bike/Power profile instance */
// Forward declaration of ANT BPWR event handler
static void ant_bpwr_evt_handler(ant_bpwr_profile_t * p_profile, ant_bpwr_evt_t event);
// Register BLE & ANT event handlers
// Forward declarations
static void ble_evt_handler(ble_evt_t const * p_ble_evt, void * p_context);
static void ant_evt_handler(ant_evt_t * p_ant_evt, void * p_context);

NRF_SDH_BLE_OBSERVER(m_ble_observer, APP_BLE_OBSERVER_PRIO, ble_evt_handler, NULL);
NRF_SDH_ANT_OBSERVER(m_ant_observer, APP_ANT_OBSERVER_PRIO, ant_evt_handler, NULL);

NRF_SDH_ANT_OBSERVER(m_ant_bpwr_observer, ANT_BPWR_ANT_OBSERVER_PRIO, ant_bpwr_disp_evt_handler, &m_ant_bpwr);

static ant_bpwr_disp_cb_t m_ant_bpwr_disp_cb;  // Display callback structure

static const ant_bpwr_disp_config_t m_ant_bpwr_profile_bpwr_disp_config =
{
    .p_cb        = &m_ant_bpwr_disp_cb,
    .evt_handler = ant_bpwr_evt_handler,
};

APP_TIMER_DEF(m_adv_restart_timer);  // Timer to restart advertising

static void advertising_start(void);  // Forward declare function

static void adv_restart_timeout_handler(void * p_context)
{
    NRF_LOG_INFO("⏰ Timer expired - Restarting BLE Advertising...");
    advertising_start();  // Restart BLE advertising
}

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


/**@brief Start advertising.
 */
static void advertising_start(void)
{
    uint32_t err_code;

    err_code = sd_ble_gap_adv_start(m_adv_handle, APP_BLE_CONN_CFG_TAG);
    if (err_code != NRF_SUCCESS && err_code != NRF_ERROR_INVALID_STATE)
    {
        APP_ERROR_HANDLER(err_code);
    }

    err_code = bsp_indication_set(BSP_INDICATE_ADVERTISING);
    APP_ERROR_CHECK(err_code);
}

static void ant_bpwr_rx_start(void)
{
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

    static const ant_channel_config_t bpwr_channel_config =
    {
        .channel_number    = ANT_BPWR_ANT_CHANNEL,
        .channel_type      = BPWR_DISP_CHANNEL_TYPE,  // ANT+ Receiver
        .ext_assign        = BPWR_EXT_ASSIGN,
        .rf_freq           = BPWR_ANTPLUS_RF_FREQ,   // Default ANT+ Frequency
        .transmission_type = ANT_BPWR_TRANS_TYPE,  
        .device_type       = 11, // ANT+ Bike Power
        .device_number     = 0,  // LISTEN TO ALL DEVICES
        .channel_period    = BPWR_MSG_PERIOD,
        .network_number    = ANTPLUS_NETWORK_NUMBER,
    };

    // Print all parameters
    NRF_LOG_INFO("🔧 ANT+ Config:");
    NRF_LOG_INFO("   - Channel Number: %d", bpwr_channel_config.channel_number);
    NRF_LOG_INFO("   - Channel Type: %d", bpwr_channel_config.channel_type);
    NRF_LOG_INFO("   - RF Freq: %d", bpwr_channel_config.rf_freq);
    NRF_LOG_INFO("   - Device Type: %d", bpwr_channel_config.device_type);
    NRF_LOG_INFO("   - Device Number: %d", bpwr_channel_config.device_number);
    NRF_LOG_INFO("   - Channel Period: %d", bpwr_channel_config.channel_period);
    NRF_LOG_INFO("   - Network Number: %d", bpwr_channel_config.network_number);

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


/**@brief Attempt to both open the ant channel and start ble advertising.
*/
static void ant_and_adv_start(void)
{
    advertising_start();
    ant_bpwr_rx_start();

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
}


/**@brief Function for initializing the GATT module.
 */
static void gatt_init(void)
{
    ret_code_t err_code = nrf_ble_gatt_init(&m_gatt, NULL);
    APP_ERROR_CHECK(err_code);
}


/**@brief GAP initialization.
 *
 * @details This function sets up all the necessary GAP (Generic Access Profile) parameters of the
 *          device including the device name, appearance, and the preferred connection parameters.
 */
static void gap_params_init(void)
{
    uint32_t                err_code;
    ble_gap_conn_params_t   gap_conn_params;
    ble_gap_conn_sec_mode_t sec_mode;

    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&sec_mode);

    err_code = sd_ble_gap_device_name_set(&sec_mode,
                                          (const uint8_t *)DEVICE_NAME,
                                          strlen(DEVICE_NAME));
    APP_ERROR_CHECK(err_code);

    err_code = sd_ble_gap_appearance_set(BLE_APPEARANCE_CYCLING_POWER_SENSOR);

    memset(&gap_conn_params, 0, sizeof(gap_conn_params));

    gap_conn_params.min_conn_interval = MIN_CONN_INTERVAL;
    gap_conn_params.max_conn_interval = MAX_CONN_INTERVAL;
    gap_conn_params.slave_latency     = SLAVE_LATENCY;
    gap_conn_params.conn_sup_timeout  = CONN_SUP_TIMEOUT;

    err_code = sd_ble_gap_ppcp_set(&gap_conn_params);
    APP_ERROR_CHECK(err_code);
}


/**@brief Advertising functionality initialization.
 *
 * @details Encodes the required advertising data and passes it to the stack.
 *          Also builds a structure to be passed to the stack when starting advertising.
 */
static void advertising_init(void)
{
    uint32_t             err_code;
    ble_advdata_t        advdata;
    ble_gap_adv_data_t   advdata_enc;
    ble_gap_adv_params_t adv_params;
    static uint8_t       advdata_buff[BLE_GAP_ADV_SET_DATA_SIZE_MAX];
    uint16_t             advdata_buff_len = BLE_GAP_ADV_SET_DATA_SIZE_MAX;
    uint8_t              flags = BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE;

    ble_uuid_t adv_uuids[] = {
        {BLE_UUID_FTMS_SERVICE, BLE_UUID_TYPE_BLE},
        {BLE_UUID_DEVICE_INFORMATION_SERVICE, BLE_UUID_TYPE_BLE}
    };

    // Build and set advertising data.
    memset(&advdata, 0, sizeof(advdata));

    advdata.name_type               = BLE_ADVDATA_FULL_NAME;
    advdata.include_appearance      = true;
    advdata.flags                   = flags;
    advdata.uuids_complete.uuid_cnt = sizeof(adv_uuids) / sizeof(adv_uuids[0]);
    advdata.uuids_complete.p_uuids  = adv_uuids;

    err_code = ble_advdata_encode(&advdata, advdata_buff, &advdata_buff_len);
    APP_ERROR_CHECK(err_code);

    m_adv_handle = BLE_GAP_ADV_SET_HANDLE_NOT_SET;

    memset(&advdata_enc, 0, sizeof(advdata_enc));

    advdata_enc.adv_data.p_data = advdata_buff;
    advdata_enc.adv_data.len    = advdata_buff_len;

    // Initialise advertising parameters (used when starting advertising).
    memset(&adv_params, 0, sizeof(adv_params));

    adv_params.primary_phy     = BLE_GAP_PHY_1MBPS;
    adv_params.properties.type = BLE_GAP_ADV_TYPE_CONNECTABLE_SCANNABLE_UNDIRECTED;
    adv_params.filter_policy   = BLE_GAP_ADV_FP_ANY;
    adv_params.interval        = APP_ADV_INTERVAL;
    adv_params.duration        = APP_ADV_DURATION;

    err_code = sd_ble_gap_adv_set_configure(&m_adv_handle, &advdata_enc, &adv_params);
    APP_ERROR_CHECK(err_code);

    NRF_LOG_INFO("Advertising UUIDs:");
    for (int i = 0; i < advdata.uuids_complete.uuid_cnt; i++) {
        NRF_LOG_INFO("UUID: 0x%04X", advdata.uuids_complete.p_uuids[i].uuid);
    }
}


/**@brief Function for handling Queued Write Module errors.
 *
 * @details A pointer to this function will be passed to each service which may need to inform the
 *          application about an error.
 *
 * @param[in]   nrf_error   Error code containing information about what went wrong.
 */
static void nrf_qwr_error_handler(uint32_t nrf_error)
{
    APP_ERROR_HANDLER(nrf_error);
}


/**@brief Initialize services that will be used by the application.
 *
 * @details Initialize the Heart Rate and Device Information services.
 */
static void services_init(void)
{
    uint32_t           err_code;
    ble_dis_init_t     dis_init;
    nrf_ble_qwr_init_t qwr_init = {0};

    // Initialize the Queued Write module.
    qwr_init.error_handler = nrf_qwr_error_handler;

    err_code = nrf_ble_qwr_init(&m_qwr, &qwr_init);
    APP_ERROR_CHECK(err_code);

    // Initialize FTMS Service
    err_code = ble_ftms_init(&m_ftms);
    if (err_code != NRF_SUCCESS) {
        NRF_LOG_ERROR("FTMS Init failed! Error code: 0x%08X", err_code);
    }
    APP_ERROR_CHECK(err_code);
    
    // Initialize Device Information Service
    memset(&dis_init, 0, sizeof(dis_init));

    ble_srv_ascii_to_utf8(&dis_init.manufact_name_str, MANUFACTURER_NAME);

    dis_init.dis_char_rd_sec = SEC_OPEN;

    err_code = ble_dis_init(&dis_init);
    APP_ERROR_CHECK(err_code);


}


/**@brief Connection Parameters Module handler.
 *
 * @details This function will be called for all events in the Connection Parameters Module which
 *          are passed to the application.
 *          @note All this function does is to disconnect. This could have been done by simply
 *                setting the disconnect_on_fail config parameter, but instead we use the event
 *                handler mechanism to demonstrate its use.
 *
 * @param[in]   p_evt   Event received from the Connection Parameters Module.
 */
static void on_conn_params_evt(ble_conn_params_evt_t * p_evt)
{
    uint32_t err_code;

    switch (p_evt->evt_type)
    {
        case BLE_CONN_PARAMS_EVT_FAILED:
            err_code = sd_ble_gap_disconnect(m_conn_handle, BLE_HCI_CONN_INTERVAL_UNACCEPTABLE);
            APP_ERROR_CHECK(err_code);
            break;

        default:
            // No implementation needed.
            break;
    }
}


/**@brief Connection Parameters module error handler.
 *
 * @param[in]   nrf_error   Error code containing information about what went wrong.
 */
static void conn_params_error_handler(uint32_t nrf_error)
{
    APP_ERROR_HANDLER(nrf_error);
}


/**@brief Initialize the Connection Parameters module.
 */
static void conn_params_init(void)
{
    uint32_t               err_code;
    ble_conn_params_init_t cp_init;

    memset(&cp_init, 0, sizeof(cp_init));

    cp_init.p_conn_params                  = NULL;
    cp_init.first_conn_params_update_delay = FIRST_CONN_PARAMS_UPDATE_DELAY;
    cp_init.next_conn_params_update_delay  = NEXT_CONN_PARAMS_UPDATE_DELAY;
    cp_init.max_conn_params_update_count   = MAX_CONN_PARAMS_UPDATE_COUNT;
    cp_init.disconnect_on_fail             = false;
    cp_init.evt_handler                    = on_conn_params_evt;
    cp_init.error_handler                  = conn_params_error_handler;

    err_code = ble_conn_params_init(&cp_init);
    APP_ERROR_CHECK(err_code);
}


void ant_evt_handler(ant_evt_t * p_ant_evt, void * p_context)
{
    NRF_LOG_INFO("📡 ANT+ Event Received - Event: %d", p_ant_evt->event);
    return;

    switch (p_ant_evt->event)
    {
        case EVENT_RX:
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

        case EVENT_CHANNEL_CLOSED:
            NRF_LOG_INFO("🔄 ANT+ Channel Closed - Restarting...");
            ant_bpwr_rx_start();  // Restart ANT+ without affecting BLE
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
            uint16_t power_watts = p_profile->page_16.instantaneous_power;
            uint8_t cadence_rpm = p_profile->common.instantaneous_cadence;

            NRF_LOG_INFO("🚴 Power: %d W, Cadence: %d RPM", power_watts, cadence_rpm);

            // ✅ Forward to BLE FTMS (if connected)
            if (m_ftms.conn_handle != BLE_CONN_HANDLE_INVALID)
            {
                ble_ftms_data_t ftms_data = {
                    .power_watts = power_watts,
                    .cadence_rpm = cadence_rpm
                };
                ble_ftms_send_indoor_bike_data(&m_ftms, &ftms_data);
            }
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


/**@brief Function for dispatching a BLE stack event to all modules with a BLE stack event handler.
 *
 * @param[in] p_ble_evt  Bluetooth stack event.
 * @param[in] p_context  Context.
 */
static void ble_evt_handler(ble_evt_t const * p_ble_evt, void * p_context)
{
    ret_code_t err_code = NRF_SUCCESS;

    switch (p_ble_evt->header.evt_id)
    {
        case BLE_GAP_EVT_CONNECTED:
            NRF_LOG_INFO("✅ BLE Connected");
            err_code = bsp_indication_set(BSP_INDICATE_CONNECTED);
            APP_ERROR_CHECK(err_code);
            m_conn_handle = p_ble_evt->evt.gap_evt.conn_handle;
            err_code = nrf_ble_qwr_conn_handle_assign(&m_qwr, m_conn_handle);
            APP_ERROR_CHECK(err_code);
            break;

        case BLE_GAP_EVT_DISCONNECTED:
            NRF_LOG_INFO("⚠️ BLE Disconnected");
            err_code = bsp_indication_set(BSP_INDICATE_IDLE);
            APP_ERROR_CHECK(err_code);
            m_conn_handle = BLE_CONN_HANDLE_INVALID;

            // Keep ANT+ Running – Do NOT close the ANT+ channel!
            // Just restart BLE advertising
            advertising_start();  
            break;

#ifndef S140
        case BLE_GAP_EVT_PHY_UPDATE_REQUEST:
        {
            ble_gap_phys_t const phys =
            {
                .rx_phys = BLE_GAP_PHY_AUTO,
                .tx_phys = BLE_GAP_PHY_AUTO,
            };

            err_code = sd_ble_gap_phy_update(p_ble_evt->evt.gap_evt.conn_handle, &phys);
            APP_ERROR_CHECK(err_code);
        } break;
#endif

#ifndef BONDING_ENABLE
        case BLE_GAP_EVT_SEC_INFO_REQUEST:
            err_code = sd_ble_gap_sec_info_reply(m_conn_handle, NULL, NULL, NULL);
            APP_ERROR_CHECK(err_code);
            break;

        case BLE_GAP_EVT_SEC_PARAMS_REQUEST:
            err_code = sd_ble_gap_sec_params_reply(m_conn_handle,
                                                   BLE_GAP_SEC_STATUS_PAIRING_NOT_SUPP,
                                                   NULL,
                                                   NULL);
            APP_ERROR_CHECK(err_code);
            break;
#endif // BONDING_ENABLE

        case BLE_GAP_EVT_ADV_SET_TERMINATED:
            if (p_ble_evt->evt.gap_evt.params.adv_set_terminated.reason ==
                BLE_GAP_EVT_ADV_SET_TERMINATED_REASON_TIMEOUT)
            {
                NRF_LOG_INFO("🔄 Advertising Timeout - Sleeping for 5 seconds before restarting...");
                err_code = bsp_indication_set(BSP_INDICATE_IDLE);
                APP_ERROR_CHECK(err_code);

                // Start a timer to wake up after 5 seconds
                err_code = app_timer_start(m_adv_restart_timer, APP_TIMER_TICKS(5000), NULL);
                APP_ERROR_CHECK(err_code);

                // Enter sleep mode (will wake up on the timer event)
                sd_app_evt_wait();
            }
            break;

#ifndef BONDING_ENABLE
        case BLE_GATTS_EVT_SYS_ATTR_MISSING:
            err_code = sd_ble_gatts_sys_attr_set(m_conn_handle,
                                                 NULL,
                                                 0,
                                                 BLE_GATTS_SYS_ATTR_FLAG_SYS_SRVCS | BLE_GATTS_SYS_ATTR_FLAG_USR_SRVCS);
            APP_ERROR_CHECK(err_code);
            break;
#endif // BONDING_ENABLE

        default:
            // No implementation needed.
            break;
    }
}


#ifdef BONDING_ENABLE
/**@brief Function for handling Peer Manager events.
 *
 * @param[in] p_evt  Peer Manager event.
 */
static void pm_evt_handler(pm_evt_t const * p_evt)
{
    pm_handler_on_pm_evt(p_evt);
    pm_handler_disconnect_on_sec_failure(p_evt);
    pm_handler_flash_clean(p_evt);

    switch (p_evt->evt_id)
    {
        case PM_EVT_PEERS_DELETE_SUCCEEDED:
            advertising_start();
            break;

        default:
            break;
    }
}


/**@brief Function for the Peer Manager initialization.
 *
 * @param[in] erase_bonds  Indicates whether bonding information should be cleared from
 *                         persistent storage during initialization of the Peer Manager.
 */
static void peer_manager_init(bool erase_bonds)
{
    ble_gap_sec_params_t sec_param;
    ret_code_t           err_code;

    err_code = pm_init();
    APP_ERROR_CHECK(err_code);

    if (erase_bonds)
    {
        err_code = pm_peers_delete();
        APP_ERROR_CHECK(err_code);
    }

    memset(&sec_param, 0, sizeof(ble_gap_sec_params_t));

    // Security parameters to be used for all security procedures.
    sec_param.bond           = SEC_PARAM_BOND;
    sec_param.mitm           = SEC_PARAM_MITM;
    sec_param.io_caps        = SEC_PARAM_IO_CAPABILITIES;
    sec_param.oob            = SEC_PARAM_OOB;
    sec_param.min_key_size   = SEC_PARAM_MIN_KEY_SIZE;
    sec_param.max_key_size   = SEC_PARAM_MAX_KEY_SIZE;
    sec_param.kdist_own.enc  = 1;
    sec_param.kdist_own.id   = 1;
    sec_param.kdist_peer.enc = 1;
    sec_param.kdist_peer.id  = 1;

    err_code = pm_sec_params_set(&sec_param);
    APP_ERROR_CHECK(err_code);

    err_code = pm_register(pm_evt_handler);
    APP_ERROR_CHECK(err_code);
}
#endif // BONDING_ENABLE


/**@brief BLE + ANT stack initialization.
 *
 * @details Initializes the SoftDevice and the stack event interrupt.
 */
static void softdevice_setup(void)
{
    ret_code_t err_code = nrf_sdh_enable_request();
    APP_ERROR_CHECK(err_code);

    ASSERT(nrf_sdh_is_enabled());
    NRF_LOG_INFO("✅ SoftDevice enabled");

    uint32_t ram_start = 0;  // Let SoftDevice decide

    err_code = nrf_sdh_ble_default_cfg_set(APP_BLE_CONN_CFG_TAG, &ram_start);
    APP_ERROR_CHECK(err_code);
    NRF_LOG_INFO("✅ BLE Config Set");

    err_code = nrf_sdh_ble_enable(&ram_start);
    if (err_code == NRF_ERROR_NO_MEM) {
        NRF_LOG_ERROR("🚨 Memory issue! SoftDevice needs more RAM.");
    }
    APP_ERROR_CHECK(err_code);
    NRF_LOG_INFO("✅ BLE Enabled");

    err_code = nrf_sdh_ant_enable();
    if (err_code != NRF_SUCCESS) {
        NRF_LOG_ERROR("🚨 ANT+ Enable FAILED: 0x%08X", err_code);
        return;
    }
    NRF_LOG_INFO("✅ ANT+ Stack Enabled");

}





/**@brief Application main function.
 */
int main(void)
{
    uint32_t err_code;

    err_code = NRF_LOG_INIT(NULL);
    APP_ERROR_CHECK(err_code);
    NRF_LOG_DEFAULT_BACKENDS_INIT();

    timers_init();
    err_code = nrf_pwr_mgmt_init();
    APP_ERROR_CHECK(err_code);

    softdevice_setup();  // Initializes BLE and ANT+ stacks

    bsp_init(BSP_INIT_LEDS | BSP_INIT_BUTTONS, NULL);
    
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

    NRF_LOG_INFO("🏁 Starting BLE and ANT+ independently...");
    
    advertising_start();  // Start BLE advertising
    ant_bpwr_rx_start();  // Start ANT+ reception

    err_code = app_timer_create(&m_adv_restart_timer, APP_TIMER_MODE_SINGLE_SHOT, adv_restart_timeout_handler);
    APP_ERROR_CHECK(err_code);

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
