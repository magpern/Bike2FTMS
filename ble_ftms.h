#ifndef BLE_FTMS_H__
#define BLE_FTMS_H__

#include <stdint.h>
#include <stdbool.h>
#include "ble.h"
#include "ble_srv_common.h"
#include "nrf_sdh_ble.h"
#include "ble_gap.h"

// FTMS Service UUID
#define BLE_UUID_FTMS_SERVICE  0x1826

// FTMS Characteristic UUIDs
#define BLE_UUID_INDOOR_BIKE_DATA_CHAR  0x2AD2
#define BLE_UUID_TRAINING_STATUS_CHAR   0x2AD3
#define BLE_UUID_FITNESS_MACHINE_STATUS_CHAR  0x2ADA

/**@brief FTMS Data Structure */
typedef struct {
    uint16_t power_watts;  // Power in Watts
    uint16_t cadence_rpm;  // Cadence in RPM
} ble_ftms_data_t;

/**@brief FTMS Training Status Structure */
typedef struct {
    uint8_t training_status;  // Current training status
} ble_ftms_training_status_t;

/**@brief FTMS Machine Status Structure */
typedef struct {
    uint8_t machine_status;  // Current machine state
} ble_ftms_machine_status_t;

/**@brief FTMS Service structure */
typedef struct {
    uint16_t                  service_handle;
    ble_gatts_char_handles_t   indoor_bike_data_handles;
    ble_gatts_char_handles_t   training_status_handles;
    ble_gatts_char_handles_t   fitness_machine_status_handles;
    uint16_t                  conn_handle;
} ble_ftms_t;

/**@brief Function for initializing the FTMS service. */
uint32_t ble_ftms_init(ble_ftms_t * p_ftms);

/**@brief Function for sending indoor bike data notifications. */
void ble_ftms_send_indoor_bike_data(ble_ftms_t * p_ftms, ble_ftms_data_t * p_data);

/**@brief Function for sending training status notifications. */
void ble_ftms_send_training_status(ble_ftms_t * p_ftms, ble_ftms_training_status_t * p_status);

/**@brief Function for sending machine status notifications. */
void ble_ftms_send_machine_status(ble_ftms_t * p_ftms, ble_ftms_machine_status_t * p_status);

#endif // BLE_FTMS_H__
