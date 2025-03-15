#ifndef BLE_CUSTOM_CONFIG_H__
#define BLE_CUSTOM_CONFIG_H__

#include <stdint.h>
#include "ble.h"
#include "ble_srv_common.h"
#define MAX_BLE_FULL_NAME_LEN 15  // Includes custom name + "_12345"
#define DEFAULT_BLE_NAME       "SatsBike"
#define BLE_NAME_MAX_LEN       8
#define DEFAULT_ANT_DEVICE_ID  0 //18465  

extern char ble_full_name[MAX_BLE_FULL_NAME_LEN];  // ✅ This is now accessible in `main.c`
extern char m_ble_name[BLE_NAME_MAX_LEN + 1];
extern uint16_t m_ant_device_id;  // ✅ This is now accessible in `main.c`

/**@brief Function for initializing the custom BLE service. */
void ble_custom_service_init(void);

/**@brief Function to load stored values from flash memory. */
void custom_service_load_from_flash(void);

/**@brief Function to handle BLE events for the custom service. */
void ble_custom_service_on_ble_evt(ble_evt_t const * p_ble_evt, void * p_context);

void save_device_config(void);
void update_ble_name(void);

#endif // BLE_CUSTOM_CONFIG_H__
