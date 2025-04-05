#ifndef REED_SENSOR_H
#define REED_SENSOR_H

#include <stdint.h>
#include "nrf_drv_gpiote.h"

// Define GPIO for reed switch
// Using P0.17 as it's available on all target boards
//
// 🧩 Physical Pin Reference for P0.17:
// - nRF52840 DK:          Pin 15 on Arduino header
// - Adafruit Feather:     D10
// - Seeed XIAO BLE:       D6
//
#define REED_SWITCH_PIN NRF_GPIO_PIN_MAP(0,17)  // Common pin P0.17 across DK, Feather, and XIAO

// Define function pointer type for callback
typedef void (*reed_sensor_callback_t)(void);

// Function prototypes
void reed_sensor_init(reed_sensor_callback_t callback);
void reed_sensor_enable(void);
void reed_sensor_disable(void);

#endif // REED_SENSOR_H
