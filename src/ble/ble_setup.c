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

#include "app_error.h"
#include "ble_ftms.h"
#include "ble_cps.h"


NRF_BLE_GATT_DEF(m_gatt);
uint8_t m_adv_handle;      /**< Advertising handle. */

void ble_power_timer_handler(void * p_context);  // ✅ Function declaration

uint16_t latest_power_watts = 0;  // Define and initialize
uint8_t latest_cadence_rpm = 0;
app_timer_id_t  m_ble_power_timer;

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