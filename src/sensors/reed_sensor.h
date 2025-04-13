#ifndef REED_SENSOR_H
#define REED_SENSOR_H

#include <stdint.h>
#include "nrf_drv_gpiote.h"

// Define GPIO for reed switch
// Using P0.04 as it's available on all target boards
//
// 🧩 Physical Pin Reference for PIN_REED_SENSOR:
// - Seeed XIAO BLE:       P0.28 (D1), second pin from top-left on the inner row
// - Pro Micro nRF52840:   P0.11 (D2), bottom row, second from left
// - nRF52840 DK (PCA10056): P1.10, available on the GPIO headers, not tied to any default functions

#define REED_SWITCH_PIN PIN_REED_SENSOR

// Define function pointer type for callback
typedef void (*reed_sensor_callback_t)(void);

// Function prototypes
void reed_sensor_init(reed_sensor_callback_t callback);
void reed_sensor_enable(void);
void reed_sensor_disable(void);

#endif // REED_SENSOR_H
