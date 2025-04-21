#ifndef KEISER_M3I_DATA_SOURCE_H
#define KEISER_M3I_DATA_SOURCE_H

#include "includes/data_source.h"
#include "common_definitions.h"
#include "nrf_log.h"
#include "app_timer.h"
#include "ble.h"
#include "ble_gap.h"

// Keiser M3i specific constants
#define KEISER_M3I_MANUFACTURER_ID 0x0102  // Keiser's manufacturer ID
#define KEISER_M3I_ADV_INTERVAL_MS 320     // Advertising interval in milliseconds
#define KEISER_M3I_ADV_TIMEOUT_MS  1000    // Timeout for not receiving data

// Keiser M3i data structure
typedef struct {
    uint16_t manufacturer_id;  // Should be 0x0102
    uint8_t  version_major;    // Version major
    uint8_t  version_minor;    // Version minor
    uint8_t  data_type;        // Data type
    uint8_t  equipment_id;     // Equipment ID
    uint16_t cadence;          // Cadence (RPM * 10)
    uint16_t heart_rate;       // Heart rate (BPM * 10)
    uint16_t power;            // Power in watts
    uint16_t calories;         // Calories
    uint8_t  duration_min;     // Duration minutes
    uint8_t  duration_sec;     // Duration seconds
    uint16_t distance;         // Distance
    uint8_t  gear;            // Gear
} keiser_m3i_data_t;

/**
 * @brief Keiser M3i specific configuration
 */
typedef struct {
    uint8_t target_mac[BLE_GAP_ADDR_LEN];  // Target MAC address to filter for
} keiser_m3i_config_t;

/**
 * @brief Initialize the Keiser M3i data source
 * 
 * @param config Configuration for the data source
 * @return true if initialization was successful
 */
bool keiser_m3i_init(data_source_config_t* config);

/**
 * @brief Start the Keiser M3i data source
 * 
 * @return true if start was successful
 */
bool keiser_m3i_start(void);

/**
 * @brief Stop the Keiser M3i data source
 */
void keiser_m3i_stop(void);

/**
 * @brief Check if the Keiser M3i data source is active
 * 
 * @return true if the source is active
 */
bool keiser_m3i_is_active(void);

/**
 * @brief Get the interface for the Keiser M3i data source
 * 
 * @return const data_source_interface_t* Interface structure
 */
const data_source_interface_t* keiser_m3i_data_source_get_interface(void);

#endif // KEISER_M3I_DATA_SOURCE_H 