#ifndef BOARD_PROMICRO_H
#define BOARD_PROMICRO_H

#ifdef __cplusplus
extern "C" {
#endif

#include "nrf_gpio.h"

// -----------------------------------------------------------------------------
// LED: P0.15 — Red onboard LED, active HIGH
// -----------------------------------------------------------------------------

#define LEDS_NUMBER       4
#define BSP_LED_0         NRF_GPIO_PIN_MAP(0,15)
#define BSP_LED_1         NRF_GPIO_PIN_MAP(0,15)
#define BSP_LED_2         NRF_GPIO_PIN_MAP(0,15)
#define BSP_LED_3         NRF_GPIO_PIN_MAP(0,15)
#define LEDS_ACTIVE_STATE 1
#define LEDS_LIST         { BSP_LED_0, BSP_LED_1, BSP_LED_2, BSP_LED_3 }
#define LEDS_INV_MASK     LEDS_MASK

#define LED_1 BSP_LED_0
#define LED_2 BSP_LED_1
#define LED_3 BSP_LED_2
#define LED_4 BSP_LED_3

// -----------------------------------------------------------------------------
// Buttons (none onboard)
// -----------------------------------------------------------------------------

#define BUTTONS_NUMBER       0
#define BUTTONS_LIST         {}
#define BUTTONS_ACTIVE_STATE NRF_GPIO_PIN_SENSE_LOW
#define BUTTON_PULL          NRF_GPIO_PIN_PULLUP

// -----------------------------------------------------------------------------
// Power control pin (P0.13 controls VCC rail or boost)
// -----------------------------------------------------------------------------

#define PIN_VCC_ENABLE NRF_GPIO_PIN_MAP(0,13)

// -----------------------------------------------------------------------------
// Battery ADC sense (AIN2 = P0.04)
// -----------------------------------------------------------------------------

#define PIN_BATTERY_ADC NRF_GPIO_PIN_MAP(0,4)

// -----------------------------------------------------------------------------
// UART (USB Serial Bridge)
// -----------------------------------------------------------------------------

#define RX_PIN_NUMBER   NRF_GPIO_PIN_MAP(1,10)  // D14
#define TX_PIN_NUMBER   NRF_GPIO_PIN_MAP(1,3)   // D15
#define CTS_PIN_NUMBER  0xFFFFFFFF
#define RTS_PIN_NUMBER  0xFFFFFFFF
#define HWFC            false

// -----------------------------------------------------------------------------
// Optional aliases for GPIO breakout (based on diagram)
// -----------------------------------------------------------------------------

#define PIN_D0   NRF_GPIO_PIN_MAP(0,8)
#define PIN_D1   NRF_GPIO_PIN_MAP(0,6)
#define PIN_D2   NRF_GPIO_PIN_MAP(0,20)
#define PIN_D3   NRF_GPIO_PIN_MAP(0,22)
#define PIN_D4   NRF_GPIO_PIN_MAP(0,24)
#define PIN_D5   NRF_GPIO_PIN_MAP(0,9)
#define PIN_D6   NRF_GPIO_PIN_MAP(1,0)
#define PIN_D7   NRF_GPIO_PIN_MAP(0,11)
#define PIN_D8   NRF_GPIO_PIN_MAP(1,6)
#define PIN_D9   NRF_GPIO_PIN_MAP(0,4)
#define PIN_D10  NRF_GPIO_PIN_MAP(0,9)
#define PIN_D11  NRF_GPIO_PIN_MAP(1,11)
#define PIN_D12  NRF_GPIO_PIN_MAP(1,13)
#define PIN_D13  NRF_GPIO_PIN_MAP(0,10)
#define PIN_D14  NRF_GPIO_PIN_MAP(1,11)
#define PIN_D15  NRF_GPIO_PIN_MAP(1,13)
#define PIN_D16  NRF_GPIO_PIN_MAP(0,10)
#define PIN_D17  NRF_GPIO_PIN_MAP(0,4)
#define PIN_D18  NRF_GPIO_PIN_MAP(1,13)
#define PIN_D19  NRF_GPIO_PIN_MAP(0,2)
#define PIN_D20  NRF_GPIO_PIN_MAP(0,29)
#define PIN_D21  NRF_GPIO_PIN_MAP(0,31)

#define PIN_REED_SENSOR       PIN_D2  // P0.11


#ifdef __cplusplus
}
#endif

#endif // BOARD_PROMICRO_H
