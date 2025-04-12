#ifndef BOARD_PROMICRO_H
#define BOARD_PROMICRO_H

#ifdef __cplusplus
extern "C" {
#endif

#include "nrf_gpio.h"

// -----------------------------------------------------------------------------
// LED: P0.15 — Red LED, active HIGH
// -----------------------------------------------------------------------------

#define LEDS_NUMBER       1
#define BSP_LED_0         NRF_GPIO_PIN_MAP(0,15)
#define LEDS_ACTIVE_STATE 0
#define LEDS_LIST         { BSP_LED_0 }
#define LEDS_INV_MASK               LEDS_MASK

#define LED_1 BSP_LED_0
#define LED_2 BSP_LED_0
#define LED_3 BSP_LED_0
#define LED_4 BSP_LED_0


// Optional: if your app or legacy code uses these
#define BSP_LED_1 LED_1
#define BSP_LED_2 LED_2
#define BSP_LED_3 LED_3
#define BSP_LED_4 LED_4

// -----------------------------------------------------------------------------
// No Buttons
// -----------------------------------------------------------------------------

#define BUTTONS_NUMBER 0
#define BUTTONS_LIST   {}
#define BUTTONS_ACTIVE_STATE NRF_GPIO_PIN_SENSE_LOW
#define BUTTON_PULL    NRF_GPIO_PIN_PULLUP

// -----------------------------------------------------------------------------
// Power control pin: P0.13 controls VCC rail
// -----------------------------------------------------------------------------

#define PIN_VCC_ENABLE NRF_GPIO_PIN_MAP(0,13)

// -----------------------------------------------------------------------------
// Battery voltage sense (ADC AIN2)
// -----------------------------------------------------------------------------

#define PIN_BATTERY_ADC NRF_GPIO_PIN_MAP(0,4)

// -----------------------------------------------------------------------------
// UART (USB-Serial bridge)
// -----------------------------------------------------------------------------

#define RX_PIN_NUMBER  NRF_GPIO_PIN_MAP(1,10)
#define TX_PIN_NUMBER  NRF_GPIO_PIN_MAP(1,3)
#define CTS_PIN_NUMBER 0xFFFFFFFF
#define RTS_PIN_NUMBER 0xFFFFFFFF
#define HWFC           false

#ifdef __cplusplus
}
#endif

#endif // BOARD_PROMICRO_H
