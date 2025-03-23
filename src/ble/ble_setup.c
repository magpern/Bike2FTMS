/**@brief GAP initialization.
 *
 * @details This function sets up all the necessary GAP (Generic Access Profile) parameters of the
 *          device including the device name, appearance, and the preferred connection parameters.
 */
#include <ble_gap.h>
#include <sdk_errors.h>
#include <nrf_sdh_ble.h>
#include <app_timer.h>
#include <nrf_log.h>
#include "ble_setup.h"
#include "common_definitions.h"

#include "ble_custom_config.h"
#include "ble_ant_scan_service.h"
#include "ble_battery_service.h"
#include "battery_measurement.h"

#include "app_error.h"
#include "ble_ftms.h"
#include "ble_cps.h"
#include <ble_dis.h>
#include <nrf_ble_qwr.h>
#include <bsp.h>
#include "nrf_sdh.h"
#include "nrf_sdh_ant.h"
#include "device_info.h"  
#include <ble_conn_params.h>
#include "ble_advdata.h"

app_timer_id_t ble_shutdown_timer;
bool ant_active = false;
bool ble_started = false;

bool ble_shutdown_timer_running = false;


NRF_BLE_GATT_DEF(m_gatt);
NRF_BLE_QWR_DEF(m_qwr);                                                              /**< Context for the Queued Write module.*/
static void ble_evt_handler(ble_evt_t const * p_ble_evt, void * p_context);
NRF_SDH_BLE_OBSERVER(m_ble_observer, APP_BLE_OBSERVER_PRIO, ble_evt_handler, NULL);
APP_TIMER_DEF(battery_timer);
APP_TIMER_DEF(m_ble_power_timer);  // ✅ Use the same timer ID as in `ble_setup.h`

volatile uint16_t m_conn_handle = BLE_CONN_HANDLE_INVALID;

uint8_t m_adv_handle;      /**< Advertising handle. */

void ble_power_timer_handler(void * p_context);  // ✅ Function declaration

uint16_t latest_power_watts = 0;  // Define and initialize
uint8_t latest_cadence_rpm = 0;

ble_ftms_t m_ftms;  // BLE FTMS Service Instance
ble_cps_t m_cps; // BLE 0x1818 Cycling Power Service Instance

void gap_params_init(void)
{
    uint32_t                err_code;
    ble_gap_conn_params_t   gap_conn_params;
    ble_gap_conn_sec_mode_t sec_mode;

    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&sec_mode);

    err_code = sd_ble_gap_device_name_set(&sec_mode,
                        (const uint8_t *)ble_full_name,  // Loaded from flash
                        strlen(ble_full_name));

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

/**@brief Function for initializing the GATT module.
 */
void gatt_init(void)
{
    ret_code_t err_code = nrf_ble_gatt_init(&m_gatt, NULL);
    APP_ERROR_CHECK(err_code);
}

void ble_power_timer_handler(void * p_context) {
    NRF_LOG_INFO("🚴 Timer Fired");

    #ifdef DEBUG  // ✅ Only flash LED in debug mode
        nrf_gpio_pin_toggle(LED3_PIN);       // Toggle LED2
    #endif

    if (m_cps.conn_handle != BLE_CONN_HANDLE_INVALID) {
        ble_cps_send_power_measurement(&m_cps, latest_power_watts);
        NRF_LOG_INFO("🚴 BLE Power Sent: %d W", latest_power_watts);
    }

    if (m_ftms.conn_handle != BLE_CONN_HANDLE_INVALID) {
        ble_ftms_data_t ftms_data = {
            .power_watts = latest_power_watts,
            .cadence_rpm = latest_cadence_rpm
        };
        ble_ftms_send_indoor_bike_data(&m_ftms, &ftms_data);
        NRF_LOG_INFO("🚴 FTMS Power Sent: %d W, Cadence: %d RPM", latest_power_watts, latest_cadence_rpm);
    }
}

void stop_ble_advertising(void)
{
    if (m_adv_handle == BLE_GAP_ADV_SET_HANDLE_NOT_SET)
    {
        NRF_LOG_WARNING("⚠️ BLE advertising handle not set, skipping stop.");
        return;
    }
    NRF_LOG_INFO("🛑 Stopping BLE power transmission timer...");
    app_timer_stop(m_ble_power_timer);
    uint32_t err_code = sd_ble_gap_adv_stop(m_adv_handle);  // ✅ Pass advertising handle

    if (err_code == NRF_SUCCESS)
    {
        NRF_LOG_INFO("📴 BLE Advertising Stopped.");
    }
    else if (err_code == NRF_ERROR_INVALID_STATE)
    {
        NRF_LOG_WARNING("⚠️ BLE Advertising was not active, no need to stop.");
    }
    else
    {
        NRF_LOG_ERROR("🚨 Failed to stop BLE advertising. Error: 0x%08X", err_code);
    }
}

/**@brief Timer callback to stop BLE advertising after grace period */
void ble_shutdown_timer_handler(void *p_context)
{
    if (!ant_active) // Ensure ANT+ did not reconnect
    {
        // 🛑 Stop BLE Power Transmission Timer
        NRF_LOG_INFO("🛑 Stopping BLE power transmission timer...");
        ble_power_timer_stop();

        NRF_LOG_WARNING("⚠️ ANT+ not found for 10 seconds. Stopping BLE...");

        stop_ble_advertising();
        ble_started = false;
    }
    ble_shutdown_timer_running = false;  // ✅ Reset timer flag
}

/**@brief BLE + ANT stack initialization.
 *
 * @details Initializes the SoftDevice and the stack event interrupt.
 */
void softdevice_setup(void)
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
void services_init(void)
{
    uint32_t err_code;
    ble_dis_init_t dis_init;
    nrf_ble_qwr_init_t qwr_init = {0};

    // Initialize the Queued Write module
    qwr_init.error_handler = nrf_qwr_error_handler;
    err_code = nrf_ble_qwr_init(&m_qwr, &qwr_init);
    APP_ERROR_CHECK(err_code);

    // Initialize FTMS Service
    err_code = ble_ftms_init(&m_ftms);
    APP_ERROR_CHECK(err_code);

    // Initialize Cycling Power Service (CPS)
    err_code = ble_cps_init(&m_cps);
    APP_ERROR_CHECK(err_code);

    // Initialize Device Information Service
    memset(&dis_init, 0, sizeof(dis_init));

    // **Dynamic Serial Number & HW Rev**
    char serial_number[20] = {0};
    char hardware_revision[20] = {0};

    get_serial_number(serial_number, sizeof(serial_number));
    get_hardware_revision(hardware_revision, sizeof(hardware_revision));

    ble_srv_ascii_to_utf8(&dis_init.manufact_name_str, "Magpern DevOps");
    ble_srv_ascii_to_utf8(&dis_init.model_num_str, "nRF52840");
    ble_srv_ascii_to_utf8(&dis_init.serial_num_str, serial_number);
    ble_srv_ascii_to_utf8(&dis_init.hw_rev_str, hardware_revision);

    char version_str[16];
    get_firmware_version(version_str, sizeof(version_str));  // Get version from build flag
    ble_srv_ascii_to_utf8(&dis_init.fw_rev_str, version_str);  // Convert to BLE UTF-8 format
    
    dis_init.dis_char_rd_sec = SEC_OPEN;

    err_code = ble_dis_init(&dis_init);
    APP_ERROR_CHECK(err_code);

    ble_ant_scan_service_init();

    // Initialize Battery monitoring ONCE
    battery_monitoring_init();  

    // Initialize Battery BLE Service (should not call battery_monitoring_init again)
    ble_battery_service_init();  

    app_timer_create(&battery_timer, APP_TIMER_MODE_REPEATED, update_battery);
    app_timer_start(battery_timer, APP_TIMER_TICKS(120000), NULL);  // Update every 2 minutes

    ble_custom_service_init();  
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
            start_ble_advertising();  
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
                NRF_LOG_INFO("🔄 Advertising Timeout - For now no connections possible.. check this...");
                err_code = bsp_indication_set(BSP_INDICATE_IDLE);
                APP_ERROR_CHECK(err_code);
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


void start_ble_advertising(void)
{
    NRF_LOG_INFO("📡 Starting BLE Advertising...");
    advertising_start();  // Use the existing function
}

/**@brief Connection Parameters module error handler.
 *
 * @param[in]   nrf_error   Error code containing information about what went wrong.
 */
static void conn_params_error_handler(uint32_t nrf_error)
{
    APP_ERROR_HANDLER(nrf_error);
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

/**@brief Initialize the Connection Parameters module.
 */
void conn_params_init(void)
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

/**@brief Advertising functionality initialization.
 *
 * @details Encodes the required advertising data and passes it to the stack.
 *          Also builds a structure to be passed to the stack when starting advertising.
 */
void advertising_init(void)
{
    uint32_t             err_code;
    ble_advdata_t        advdata;
    ble_gap_adv_data_t   advdata_enc;
    ble_gap_adv_params_t adv_params;
    static uint8_t       advdata_buff[BLE_GAP_ADV_SET_DATA_SIZE_MAX];
    uint16_t             advdata_buff_len = BLE_GAP_ADV_SET_DATA_SIZE_MAX;
    uint8_t              flags = BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE;

    ble_uuid_t adv_uuids[] = {
        {BLE_UUID_FTMS_SERVICE, BLE_UUID_TYPE_BLE},          // ✅ FTMS Service
        {BLE_UUID_CYCLING_POWER_SERVICE, BLE_UUID_TYPE_BLE}, // ✅ Cycling Power Service
        {ANT_SCAN_SERVICE_UUID, BLE_UUID_TYPE_BLE} 
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
    adv_params.duration        = BLE_GAP_ADV_TIMEOUT_GENERAL_UNLIMITED;
    
    err_code = sd_ble_gap_adv_set_configure(&m_adv_handle, &advdata_enc, &adv_params);
    APP_ERROR_CHECK(err_code);

    NRF_LOG_INFO("Advertising UUIDs:");
    for (int i = 0; i < advdata.uuids_complete.uuid_cnt; i++) {
        NRF_LOG_INFO("UUID: 0x%04X", advdata.uuids_complete.p_uuids[i].uuid);
    }
}

// 3) Create function
void ble_power_timer_create(void)
{
    ret_code_t err_code;

    err_code = app_timer_create(&m_ble_power_timer,
                                APP_TIMER_MODE_REPEATED,
                                ble_power_timer_handler);
    APP_ERROR_CHECK(err_code);

    NRF_LOG_INFO("✅ BLE power timer created");
}

// 4) Start function
void ble_power_timer_start(void)
{
    ret_code_t err_code;

    // Example: Fire every 2 seconds
    err_code = app_timer_start(m_ble_power_timer, APP_TIMER_TICKS(1000), NULL);
    APP_ERROR_CHECK(err_code);

    NRF_LOG_INFO("✅ BLE power timer started");
}

// 5) Stop function
void ble_power_timer_stop(void)
{
    ret_code_t err_code = app_timer_stop(m_ble_power_timer);

    // If it’s already stopped, it returns NRF_SUCCESS or NRF_ERROR_INVALID_STATE
    if ((err_code != NRF_SUCCESS) && (err_code != NRF_ERROR_INVALID_STATE))
    {
        APP_ERROR_CHECK(err_code);
    }

    NRF_LOG_INFO("🛑 BLE power timer stopped");
}