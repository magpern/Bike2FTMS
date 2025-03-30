#include <stdint.h>  // ✅ For uint16_t, etc.
#include <stdbool.h> // ✅ Fixes "identifier bool is undefined"
#include "ble_types.h"
#include "nrf_gpio.h"

#define LED1_PIN NRF_GPIO_PIN_MAP(0,13)  // LED1 on nRF52840 DK
#define LED2_PIN NRF_GPIO_PIN_MAP(0,14)  // LED2 on nRF52840 DK
#define LED3_PIN NRF_GPIO_PIN_MAP(0,15)  // LED3 on nRF52840 DK
#define LED4_PIN NRF_GPIO_PIN_MAP(0,16)  // LED4 on nRF52840 DK

#define MANUFACTURER_NAME "Magpern Devops"  // Manufacturer name

#define APP_ADV_INTERVAL 40       // BLE advertising interval (25 ms)
#define APP_ADV_DURATION 18000    // BLE advertising duration (seconds)

#define APP_BLE_CONN_CFG_TAG 1    // BLE stack configuration tag

#define IS_SRVC_CHANGED_CHARACT_PRESENT 0  // Whether service change characteristic is present

#define MIN_CONN_INTERVAL (80 / 2)  // 50 ms
#define MAX_CONN_INTERVAL (80)      // 100 ms
#define SLAVE_LATENCY 0             // No slave latency
#define CONN_SUP_TIMEOUT (4 * 100)  // 4 seconds timeout

#define FIRST_CONN_PARAMS_UPDATE_DELAY APP_TIMER_TICKS(5000)  // 5s delay before first param update
#define NEXT_CONN_PARAMS_UPDATE_DELAY  APP_TIMER_TICKS(30000) // 30s between param updates
#define MAX_CONN_PARAMS_UPDATE_COUNT   3  // Number of retries

#define SEC_PARAM_TIMEOUT 30  // Pairing request timeout (seconds)
#define SEC_PARAM_BOND     1  // Enable bonding
#define SEC_PARAM_MITM     0  // No MITM protection
#define SEC_PARAM_IO_CAPABILITIES BLE_GAP_IO_CAPS_NONE  // No I/O capabilities
#define SEC_PARAM_OOB     0   // No Out Of Band pairing
#define SEC_PARAM_MIN_KEY_SIZE 7
#define SEC_PARAM_MAX_KEY_SIZE 16

#define DEAD_BEEF 0xDEADBEEF  // Error code for stack dump debugging

#define ANT_BPWR_ANT_CHANNEL 1   // Channel used for ANT+ communication MUST not be 0
#define ANT_BPWR_DEVICE_NUMBER 18465  // Default ANT+ device number
#define ANT_BPWR_TRANS_TYPE 5  // Transmission Type
#define ANTPLUS_NETWORK_NUMBER 0  // Network number

#define ANT_PLUS_NETWORK_KEY ((uint8_t[8]){0xB9, 0xA5, 0x21, 0xFB, 0xBD, 0x72, 0xC3, 0x45})  // ANT+ Key

#define WAKEUP_BUTTON_ID                0                                            /**< Button used to wake up the application. */
#define BOND_DELETE_ALL_BUTTON_ID       1                                            /**< Button used for deleting all bonded centrals during startup. */


#define CONN_INTERVAL_BASE              80                                           /**< Definition of 100 ms, when 1 unit is 1.25 ms. */
#define SECOND_10_MS_UNITS              100      

#define RESET_DUPLICATE_COUNTER_EVERY_N_MESSAGE 2  // Force send after 5 same values

extern bool ble_started;  // Track if BLE is running
extern bool ant_active;   // Track if ANT+ is running
extern bool ble_shutdown_timer_running; // If BLE shutdown timer is active
extern volatile uint16_t m_conn_handle; // BLE connection handle

