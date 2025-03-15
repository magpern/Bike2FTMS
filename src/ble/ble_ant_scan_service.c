#include "ble_ant_scan_service.h"
#include "ble_srv_common.h"
#include "nrf_log.h"
#include "app_error.h"
#include "ant_interface.h"
#include "ant_bpwr.h"
#include "ble_custom_config.h"
#include "nrf_sdh_ble.h"
#include "common_definitions.h"
#include <stdlib.h>  // ✅ Required for rand()

#define ANT_SCAN_SERVICE_UUID          0x1600
#define SCAN_CONTROL_CHAR_UUID         0x1601
#define SCAN_RESULTS_CHAR_UUID         0x1602
#define SELECT_DEVICE_CHAR_UUID        0x1603

#define MAX_ANT_DEVICES 10  // Store up to 10 recent devices

#define MOCK_SCANING

static uint16_t m_service_handle;
static ble_gatts_char_handles_t scan_control_handles;
static ble_gatts_char_handles_t scan_results_handles;
static ble_gatts_char_handles_t select_device_handles;

static ant_device_t found_devices[MAX_ANT_DEVICES];
static uint8_t num_found_devices = 0;
static bool scanning_active = false;

// Forward declarations
static void send_scan_result(uint16_t device_id, int8_t rssi);
static bool is_device_in_list(uint16_t device_id);
static void ant_scan_callback(uint16_t device_id, int8_t rssi);

static void ble_ant_scan_service_on_ble_evt(ble_evt_t const *p_ble_evt, void *p_context);
NRF_SDH_BLE_OBSERVER(m_ant_scan_service_observer, APP_BLE_OBSERVER_PRIO, ble_ant_scan_service_on_ble_evt, NULL);


/* This is for mocking only */

// Function prototype
#ifdef MOCK_SCANING
    #include "app_timer.h"
    #include "nrf_delay.h"
    static void send_random_device(void *p_context);

    // Define a timer to send devices asynchronously
    APP_TIMER_DEF(m_ant_scan_timer);
    static uint8_t devices_sent = 0;

    /**@brief Timer callback to send a random ANT+ device */
    void send_random_device(void *p_context) {
        if (devices_sent >= 10) {
            scanning_active = false;
            return;  // Stop after 10 devices
        }

        // Generate random device ID (1 to 35000)
        uint16_t device_id = (rand() % 35000) + 1;

        // Generate random RSSI (-30 to -90 dBm)
        int8_t rssi = -30 - (rand() % 60);

        // Send to callback
        ant_scan_callback(device_id, rssi);

        devices_sent++;

        // Schedule the next device transmission between 500ms and 3 seconds
        uint32_t delay_ms = 500 + (rand() % 2500);  // 500ms to 3000ms
        app_timer_start(m_ant_scan_timer, APP_TIMER_TICKS(delay_ms), NULL);
    }
#endif

/* End mock*/

/**@brief Function to check if a device ID is already in the list */
static bool is_device_in_list(uint16_t device_id) {
    for (uint8_t i = 0; i < num_found_devices; i++) {
        if (found_devices[i].device_id == device_id) {
            return true;
        }
    }
    return false;
}

/**@brief Send an individual scan result as a BLE notification */
static void send_scan_result(uint16_t device_id, int8_t rssi) {
    if (m_conn_handle == BLE_CONN_HANDLE_INVALID) return;

    uint8_t data[3];  // ✅ Only 3 bytes: Device ID (2B) + RSSI (1B)
    data[0] = device_id & 0xFF;
    data[1] = (device_id >> 8) & 0xFF;
    data[2] = rssi;

    ble_gatts_hvx_params_t params = {0};
    params.handle = scan_results_handles.value_handle;  // ✅ Ensure this is the correct handle
    params.type = BLE_GATT_HVX_NOTIFICATION;
    params.p_data = data;
    params.p_len = (uint16_t[]){sizeof(data)};

    ret_code_t err_code = sd_ble_gatts_hvx(m_conn_handle, &params);
    if (err_code == NRF_SUCCESS) {
        NRF_LOG_INFO("📡 Sent ANT+ Device ID: %d, RSSI: %d", device_id, rssi);
    } else {
        NRF_LOG_WARNING("⚠️ Failed to send notification: 0x%08X", err_code);
    }
}


/**@brief Callback function for when an ANT+ device is detected */
static void ant_scan_callback(uint16_t device_id, int8_t rssi) {
    if (num_found_devices >= MAX_ANT_DEVICES) return;  // Prevent overflow

    if (!is_device_in_list(device_id)) {
        NRF_LOG_INFO("📡 Found ANT+ Device ID: %d, RSSI: %d", device_id, rssi);
        found_devices[num_found_devices].device_id = device_id;
        found_devices[num_found_devices].rssi = rssi;
        num_found_devices++;

        send_scan_result(device_id, rssi); // Send immediately
    }
}

/**@brief Function to start ANT+ scanning */
void ant_scan_start(void) {
    if (scanning_active) return;

    num_found_devices = 0;
    scanning_active = true;

    NRF_LOG_INFO("🔍 Starting ANT+ Scan...");

    // Simulated: Call `ant_scan_callback(device_id, rssi);` for real scan results
    
    #ifdef MOCK_SCANING
        // Start sending devices asynchronously
        app_timer_create(&m_ant_scan_timer, APP_TIMER_MODE_SINGLE_SHOT, send_random_device);
        send_random_device(NULL);  // Send the first device immediately
    #else
        // Real ANT+ scanning
    #endif

}

/**@brief Send BLE name as a notification */
static void send_ble_name(void) {
    if (m_conn_handle == BLE_CONN_HANDLE_INVALID) return;

    uint8_t name_length = strlen(m_ble_name);
    if (name_length > BLE_NAME_MAX_LEN) name_length = BLE_NAME_MAX_LEN;  // ✅ Ensure it doesn't exceed max length

    uint8_t data[1 + name_length];  // ✅ First byte = Name Length
    data[0] = name_length;
    memcpy(&data[1], m_ble_name, name_length);  // ✅ Copy BLE name

    ble_gatts_hvx_params_t params = {0};
    params.handle = scan_results_handles.value_handle;
    params.type = BLE_GATT_HVX_NOTIFICATION;
    params.p_data = data;
    
    uint16_t length = sizeof(data);
    params.p_len = &length;

    ret_code_t err_code = sd_ble_gatts_hvx(m_conn_handle, &params);
    if (err_code == NRF_SUCCESS) {
        NRF_LOG_INFO("📡 Sent BLE Name: %.*s", name_length, m_ble_name);
    } else {
        NRF_LOG_WARNING("⚠️ Failed to send BLE name: 0x%08X", err_code);
    }
}


/**@brief BLE write handler for start scan & device selection */
static void on_write(ble_evt_t const *p_ble_evt) {
    ble_gatts_evt_write_t const *p_evt_write = &p_ble_evt->evt.gatts_evt.params.write;

    if (p_evt_write->handle == scan_control_handles.value_handle && p_evt_write->len == 1) {
        if (p_evt_write->handle == scan_control_handles.value_handle && p_evt_write->len == 1) {
            if (p_evt_write->data[0] == 0x01) {
                NRF_LOG_INFO("📡 BLE Triggered ANT+ Scan");
                ant_scan_start();
            } else if (p_evt_write->data[0] == 0x02) {
                NRF_LOG_INFO("📡 BLE Triggered Stop ANT+ Scan");
                scanning_active = false;
                devices_sent = 0;
            } else if (p_evt_write->data[0] == 0x03) {  // ✅ New command to return BLE name
                NRF_LOG_INFO("📡 BLE Requested Device Name");
                send_ble_name();
            }
        } 
        
    } else if (p_evt_write->handle == select_device_handles.value_handle && p_evt_write->len == 2) {
        uint16_t selected_device_id = (p_evt_write->data[0] | (p_evt_write->data[1] << 8));
        NRF_LOG_INFO("✅ Selected Device ID: %d", selected_device_id);

        m_ant_device_id = selected_device_id;
        update_ble_name();
        //save_device_config();

        // Notify BLE with updated name
        send_scan_result(m_ant_device_id, 0);
    }
}

/**@brief Function for handling BLE events in the ANT+ Scan Service */
static void ble_ant_scan_service_on_ble_evt(ble_evt_t const *p_ble_evt, void *p_context) {
    if (p_ble_evt->header.evt_id == BLE_GATTS_EVT_WRITE) {
        on_write(p_ble_evt);  // Calls the appropriate handler for written characteristics
    }
}

/**@brief Function to initialize the ANT+ Scan Service */
void ble_ant_scan_service_init(void) {
    ble_uuid_t ble_uuid;
    ble_uuid.type = BLE_UUID_TYPE_BLE;
    ble_uuid.uuid = ANT_SCAN_SERVICE_UUID;

    sd_ble_gatts_service_add(BLE_GATTS_SRVC_TYPE_PRIMARY, &ble_uuid, &m_service_handle);

    // Add Scan Control Characteristic
    ble_add_char_params_t scan_control_params = {0};
    scan_control_params.uuid = SCAN_CONTROL_CHAR_UUID;
    scan_control_params.uuid_type = BLE_UUID_TYPE_BLE;
    scan_control_params.init_len = 1;
    scan_control_params.max_len = 1;
    scan_control_params.char_props.write = 1;
    scan_control_params.write_access = SEC_OPEN;
    characteristic_add(m_service_handle, &scan_control_params, &scan_control_handles);

    // ✅ Add Scan Results Characteristic (Notify-Only)
    ble_add_char_params_t scan_results_params = {0};
    scan_results_params.uuid = SCAN_RESULTS_CHAR_UUID;
    scan_results_params.uuid_type = BLE_UUID_TYPE_BLE;
    scan_results_params.init_len = 1;   // Start empty (e.g., 0x00)
    scan_results_params.max_len = 100;  // Allow multiple devices
    scan_results_params.char_props.notify = 1;  // ✅ Enable NOTIFY (No Read)
    scan_results_params.cccd_write_access = SEC_OPEN;  // ✅ Allow client to enable notifications
    scan_results_params.is_var_len = true;  // ✅ Support variable-length data

    // ✅ Initialize with an empty value
    uint8_t initial_value[1] = {0};  // Empty value
    scan_results_params.p_init_value = initial_value;

    characteristic_add(m_service_handle, &scan_results_params, &scan_results_handles);


    // Add Select Device Characteristic
    ble_add_char_params_t select_device_params = {0};
    select_device_params.uuid = SELECT_DEVICE_CHAR_UUID;
    select_device_params.uuid_type = BLE_UUID_TYPE_BLE;
    select_device_params.init_len = 2;
    select_device_params.max_len = 2;
    select_device_params.char_props.write = 1;
    select_device_params.write_access = SEC_OPEN;
    characteristic_add(m_service_handle, &select_device_params, &select_device_handles);

    NRF_LOG_INFO("✅ ANT+ Scan Service Initialized");
}
