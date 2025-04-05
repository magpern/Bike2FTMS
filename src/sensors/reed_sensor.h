#ifndef REED_SENSOR_H
#define REED_SENSOR_H

#include <stdint.h>
#include "nrf_drv_gpiote.h"

// Define GPIO for reed switch
// Using P0.04 as it's available on all target boards
//
// 🧩 Physical Pin Reference for P0.04:
// - Seeed XIAO BLE:       A4, fifth from left top
// - Pro Micro nRF52840:   Bottom-left, one up, labeled "104"
// - nRF52840 DK:          Pin 4 on Arduino header
//
#define REED_SWITCH_PIN NRF_GPIO_PIN_MAP(0,4)

// Define function pointer type for callback
typedef void (*reed_sensor_callback_t)(void);

// Function prototypes
void reed_sensor_init(reed_sensor_callback_t callback);
void reed_sensor_enable(void);
void reed_sensor_disable(void);

#endif // REED_SENSOR_H
