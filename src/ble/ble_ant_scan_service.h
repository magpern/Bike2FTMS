#ifndef BLE_ANT_SCAN_SERVICE_H__
#define BLE_ANT_SCAN_SERVICE_H__

#include <stdint.h>
#include "ble.h"
#include "ble_srv_common.h"
#define MAX_ANT_DEVICES 10  // Reasonable limit
#define ANT_SCAN_SERVICE_UUID          0x1600
#define SCAN_CONTROL_CHAR_UUID         0x1601
#define SCAN_RESULTS_CHAR_UUID         0x1602
#define SELECT_DEVICE_CHAR_UUID        0x1603

// Some IDs for clarity (adapt as needed):
#define SCANNING_CHANNEL_NUMBER       1    // Use channel 1 for scanning

/**@brief Structure to store scanned ANT+ devices */
typedef struct {
    uint16_t device_id;
    int8_t rssi;
} ant_device_t;

/**@brief Function to initialize the ANT+ Scan Service */
void ble_ant_scan_service_init(void);

/**@brief Function to start ANT+ scanning (triggered via BLE) */
void update_battery(void *p_context);

#endif // BLE_ANT_SCAN_SERVICE_H__
