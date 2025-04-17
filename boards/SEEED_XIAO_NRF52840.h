/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2020 Drew Vigne
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */


#ifndef _SEEED_XIAO_NRF52840_H_
#define _SEEED_XIAO_NRF52840_H_

#ifdef __cplusplus
extern "C" {
#endif

#ifndef BSP_SIMPLE
#define BSP_SIMPLE
#endif

#include "nrf_gpio.h"

// LED Definitions (for the RGB LED on the XIAO nRF52840)
#define LEDS_NUMBER                 4  // XIAO has an RGB LED, which is treated as 3 LEDs (but our code should work on a 4 led device)
#define LED_START                   NRF_GPIO_PIN_MAP(0,26)  // Start at Red pin
#define LED_1                       NRF_GPIO_PIN_MAP(0,26)  // Red LED pin
#define LED_2                       NRF_GPIO_PIN_MAP(0,6)  // Blue LED pin
#define LED_3                       NRF_GPIO_PIN_MAP(0,30)  // Green LED pin
#define LED_4                       LED_3
#define LED_STOP                    NRF_GPIO_PIN_MAP(0,26)

#define LED_RED                     LED_1  // Red LED pin
#define LED_BLUE                    LED_3  // Blue LED pin  
#define LED_GREEN                   LED_2  // Green LED pin

#define LEDS_LIST                   { LED_1, LED_2, LED_3 , LED_4 }
#define LEDS_ACTIVE_STATE           0  // XIAO LEDs are active low
#define LED_BUILTIN                 LED_1  // Default built-in LED is the red one

// Button Definitions
#define BUTTONS_NUMBER              0  // No built-in buttons on XIAO
#define BUTTONS_LIST                {} // Empty as no buttons
#define BUTTON_ACTIVE_STATE			NRF_GPIO_PIN_NOSENSE

// IO Pin Definitions (Digital Pins)
#define RX_PIN_NUMBER               NRF_GPIO_PIN_MAP(1,12)   // UART RX
#define TX_PIN_NUMBER               NRF_GPIO_PIN_MAP(1,11)   // UART TX

#define PIN_SERIAL1_RX              (7)  // Matches XIAO UART pin definitions
#define PIN_SERIAL1_TX              (6)

// Analog Pin Definitions
#define PIN_A0                      NRF_GPIO_PIN_MAP(0,2)   // Analog Pin 0
#define PIN_A1                      NRF_GPIO_PIN_MAP(0,3)   // Analog Pin 1
#define PIN_A2                      NRF_GPIO_PIN_MAP(0,28)
#define PIN_A3                      NRF_GPIO_PIN_MAP(0,29)
#define PIN_A4                      PIN_WIRE_SDA
#define PIN_A5                      PIN_WIRE_SCL

// -----------------------------------------------------------------------------
// Digital Pin Aliases (D0–D10)
// -----------------------------------------------------------------------------

#define PIN_D0                PIN_A0      
#define PIN_D1                PIN_A1     
#define PIN_D2                PIN_A2     
#define PIN_D3                PIN_A3     
#define PIN_D4                PIN_A4
#define PIN_D5                PIN_A5
#define PIN_D6                TX_PIN_NUMBER
#define PIN_D7                RX_PIN_NUMBER 
#define PIN_D8                PIN_SPI_SCK
#define PIN_D9                PIN_SPI_MISO  
#define PIN_D10               PIN_SPI_MOSI  

#define PIN_REED_SENSOR       PIN_D2  // P0.28

// I2C Definitions
#define PIN_WIRE_SDA                NRF_GPIO_PIN_MAP(0,4)   // I2C SDA
#define PIN_WIRE_SCL                NRF_GPIO_PIN_MAP(0,5)   // I2C SCL

// SPI Interface
#define PIN_SPI_MISO NRF_GPIO_PIN_MAP(1,14)
#define PIN_SPI_MOSI NRF_GPIO_PIN_MAP(1,15)
#define PIN_SPI_SCK  NRF_GPIO_PIN_MAP(1,13)

// QSPI Pins (On-board Flash Memory)
#define PIN_QSPI_SCK                NRF_GPIO_PIN_MAP(0,21)
#define PIN_QSPI_CS                 NRF_GPIO_PIN_MAP(0,25)
#define PIN_QSPI_IO0                NRF_GPIO_PIN_MAP(0,20)
#define PIN_QSPI_IO1                NRF_GPIO_PIN_MAP(0,24)
#define PIN_QSPI_IO2                NRF_GPIO_PIN_MAP(0,22)
#define PIN_QSPI_IO3                NRF_GPIO_PIN_MAP(0,23)

#define EXTERNAL_FLASH_DEVICES      (P25Q16H)  // XIAO Flash device
#define EXTERNAL_FLASH_USE_QSPI

#ifdef __cplusplus
}
#endif

#endif // _SEEED_XIAO_NRF52840_H_