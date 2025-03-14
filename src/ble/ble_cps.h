#ifndef BLE_CPS_H__
#define BLE_CPS_H__

#include <stdint.h>
#include "ble.h"
#include "ble_srv_common.h"

#define BLE_CPS_FEATURE_WHEEL_REV     (1 << 0)
#define BLE_CPS_FEATURE_CRANK_REV     (1 << 1)
#define BLE_CPS_FEATURE_POWER_BALANCE (1 << 2)

#define BLE_UUID_CYCLING_POWER_SERVICE   0x1818  /**< Cycling Power Service UUID. */
#define BLE_UUID_POWER_MEASUREMENT_CHAR  0x2A63  /**< Power Measurement Characteristic UUID. */
#define BLE_UUID_CYCLING_POWER_FEATURE_CHAR 0x2A65  /**< Cycling Power Feature Characteristic UUID. */
#define BLE_UUID_SENSOR_LOCATION_CHAR    0x2A5D  /**< Sensor Location Characteristic UUID. */

/**@brief Cycling Power Service structure. */
typedef struct {
    uint16_t conn_handle;
    uint16_t service_handle;
    ble_gatts_char_handles_t power_measurement_handles;
    ble_gatts_char_handles_t feature_handles;
    ble_gatts_char_handles_t sensor_location_handles;  // Added for Sensor Location characteristic
} ble_cps_t;

/**@brief Function for initializing the Cycling Power Service. */
uint32_t ble_cps_init(ble_cps_t * p_cps);

/**@brief Function for sending a Cycling Power Measurement notification. */
void ble_cps_send_power_measurement(ble_cps_t * p_cps, uint16_t power_watts);

#endif // BLE_CPS_H__
