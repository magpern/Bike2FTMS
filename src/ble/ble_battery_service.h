#ifndef BLE_BATTERY_SERVICE_H__
#define BLE_BATTERY_SERVICE_H__

#include <stdint.h>
#include "ble.h"
#include "ble_srv_common.h"

#define BATTERY_SERVICE_UUID  0x180F
#define BATTERY_LEVEL_CHAR_UUID  0x2A19
#define BATTERY_POWER_STATE_CHAR_UUID 0x2A1A


typedef struct {
    uint16_t service_handle;
    ble_gatts_char_handles_t battery_level_handles;
    ble_gatts_char_handles_t battery_power_state_handles;  // ✅ Use correct handle for power state
    uint16_t conn_handle;
} ble_battery_t;

extern ble_battery_t m_battery_service; 


/**@brief Function to initialize the Battery Service */
void ble_battery_service_init(void);  // ✅ Remove `p_battery_service` parameter

/**@brief Function to update battery level */
void ble_battery_update(uint8_t battery_level, uint16_t voltage_mv, uint8_t power_state);

#endif // BLE_BATTERY_SERVICE_H__
