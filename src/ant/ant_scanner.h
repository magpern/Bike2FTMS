#ifndef ANT_SCANNER_H
#define ANT_SCANNER_H

#include <stdint.h>
#include <stdbool.h>

// Callback function type for device discovery
typedef void (*ant_device_callback_t)(uint16_t device_id, int8_t rssi);

// Maximum number of devices to store
#define MAX_ANT_DEVICES 10

// Maximum scan duration in milliseconds
#define MAX_SCAN_DURATION_MS 10000

/**@brief Initialize the ANT scanner service.
 * 
 * This function initializes the ANT scanner service and registers the callback
 * for device discovery notifications. Must be called before using any other
 * scanner functions.
 * 
 * @param[in] callback Function to be called when a new device is discovered.
 *                      If NULL, no callback will be registered.
 * 
 * @return NRF_SUCCESS if initialization was successful, otherwise an error code.
 */
uint32_t ant_scanner_init(ant_device_callback_t callback);

/**@brief Start scanning for ANT+ devices.
 * 
 * This function starts scanning for ANT+ devices and will automatically stop
 * after MAX_SCAN_DURATION_MS milliseconds.
 * 
 * @return NRF_SUCCESS if scanning was started successfully, otherwise an error code.
 */
uint32_t ant_scanner_start(void);

/**@brief Stop scanning for ANT+ devices.
 * 
 * This function stops the scanning process immediately.
 */
void ant_scanner_stop(void);

/**@brief Check if scanning is currently active.
 * 
 * @return true if scanning is active, false otherwise.
 */
bool ant_scanner_is_active(void);

#endif // ANT_SCANNER_H
