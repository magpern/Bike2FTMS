#ifndef BLE_ANT_SCAN_SERVICE_H__
#define BLE_ANT_SCAN_SERVICE_H__

#include <stdint.h>
#include "ble.h"
#include "ble_srv_common.h"
#define MAX_ANT_DEVICES 10  // Reasonable limit

/**@brief Structure to store scanned ANT+ devices */
typedef struct {
    uint16_t device_id;
    int8_t rssi;
} ant_device_t;

/**@brief Function to initialize the ANT+ Scan Service */
void ble_ant_scan_service_init(void);

/**@brief Function to start ANT+ scanning (triggered via BLE) */
void ant_scan_start(void);

#endif // BLE_ANT_SCAN_SERVICE_H__
