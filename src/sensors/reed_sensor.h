#ifndef REED_SENSOR_H
#define REED_SENSOR_H

#include <stdint.h>
#include "nrf_drv_gpiote.h"

// Define GPIO for reed switch
#define REED_SWITCH_PIN 2  // P0.02 on nRF52840 DK

// Define function pointer type for callback
typedef void (*reed_sensor_callback_t)(void);

// Function prototypes
void reed_sensor_init(reed_sensor_callback_t callback);
void reed_sensor_enable(void);
void reed_sensor_disable(void);

#endif // REED_SENSOR_H
