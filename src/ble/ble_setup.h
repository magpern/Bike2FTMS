#ifndef GAP_GATT_INIT_H
#define GAP_GATT_INIT_H

#include <ble_gap.h>
#include <sdk_errors.h>
#include "common_definitions.h"
#include "ble_custom_config.h"
#include <nrf_ble_gatt.h>
#include "app_timer.h"
#include "ble_ftms.h"
#include "ble_cps.h"

#ifdef __cplusplus
extern "C" {
#endif

extern uint8_t m_adv_handle;
extern ble_ftms_t m_ftms;  // BLE FTMS Service Instance
extern ble_cps_t m_cps; // BLE 0x1818 Cycling Power Service Instance

extern uint16_t latest_power_watts;  // Accessible globally
extern uint8_t latest_cadence_rpm;

extern app_timer_id_t ble_shutdown_timer;  
extern bool ant_active;
extern bool ble_shutdown_timer_running;
extern void ble_shutdown_timer_handler(void *p_context);
extern void stop_ble_advertising(void);

void start_ble_advertising(void);
void softdevice_setup(void);
void services_init(void);
void conn_params_init(void);
void advertising_init(void);

/**@brief Initializes GAP parameters including device name, appearance, and connection parameters.
 */
void gap_params_init(void);

/**@brief Initializes the GATT module.
 */
void gatt_init(void);

#ifdef __cplusplus
}
#endif

#endif // GAP_GATT_INIT_H
