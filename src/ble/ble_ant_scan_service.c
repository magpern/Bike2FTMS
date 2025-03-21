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
        ret_code_t err_code = app_timer_start(m_ant_scan_timer, APP_TIMER_TICKS(delay_ms), NULL);
        APP_ERROR_CHECK(err_code);
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

    num_found_devices = 0;  // ✅ Reset the found devices list
    scanning_active = true; // ✅ Indicate scan is in progress
    devices_sent = 0;       // ✅ Reset the number of devices sent
    
    NRF_LOG_INFO("🔍 Starting ANT+ Scan...");

    #ifdef MOCK_SCANING
        static bool timer_created = false;  // ✅ Track if timer was created

        // ✅ Create the timer ONLY ONCE
        if (!timer_created) {
            ret_code_t err_code = app_timer_create(&m_ant_scan_timer, APP_TIMER_MODE_SINGLE_SHOT, send_random_device);
            APP_ERROR_CHECK(err_code);
            timer_created = true;  // ✅ Mark timer as created
        }
        // ✅ Send the first device immediately
        send_random_device(NULL);
    #else
        // ✅ Start actual ANT+ scanning here (if not mocked)
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

/**@brief Function to send the current ANT+ Device ID as a notification */
static void send_current_ant_device_id(void) {
    if (m_conn_handle == BLE_CONN_HANDLE_INVALID) return;

    uint8_t data[2];
    data[0] = m_ant_device_id & 0xFF;  // LSB
    data[1] = (m_ant_device_id >> 8) & 0xFF;  // MSB

    ble_gatts_hvx_params_t params = {0};
    params.handle = scan_results_handles.value_handle;  // ✅ Send as a scan result
    params.type = BLE_GATT_HVX_NOTIFICATION;
    params.p_data = data;
    uint16_t len = sizeof(data);
    params.p_len = &len;

    ret_code_t err_code = sd_ble_gatts_hvx(m_conn_handle, &params);
    if (err_code == NRF_SUCCESS) {
        NRF_LOG_INFO("📡 Sent Current ANT+ Device ID: %d", m_ant_device_id);
    } else {
        NRF_LOG_WARNING("⚠️ Failed to send ANT+ Device ID: 0x%08X", err_code);
    }
}

/**@brief BLE write handler for start scan & device selection */
static void on_write(ble_evt_t const *p_ble_evt) {
    ble_gatts_evt_write_t const *p_evt_write = &p_ble_evt->evt.gatts_evt.params.write;

    if (p_evt_write->handle == scan_control_handles.value_handle && p_evt_write->len == 1) {
        uint8_t command = p_evt_write->data[0];

        switch (command) {
            case 0x01:  // 🔍 Start or Restart Scanning
                NRF_LOG_INFO("📡 BLE Triggered ANT+ Scan (Restarting)");

                // ✅ Clear the list of found devices
                memset(found_devices, 0, sizeof(found_devices));
                num_found_devices = 0;

                ant_scan_start();
                break;

            case 0x02:  // 🛑 Stop Scan
            case 0x03:  // 📡 Get BLE Name
            case 0x04:  // 🔢 Get ANT+ Device ID
                NRF_LOG_INFO("🛑 BLE Stopping Scan (Command: 0x%02X)", command);
                scanning_active = false;
                num_found_devices = 0;
                
                #ifdef MOCK_SCANING
                    app_timer_stop(m_ant_scan_timer);  // ✅ Stop the timer if running
                #endif

                if (command == 0x03) {
                    send_ble_name();
                } else if (command == 0x04) {
                    send_current_ant_device_id();
                }
                break;

            default:
                NRF_LOG_WARNING("⚠️ Unknown Command: 0x%02X", command);
                break;
        }
    } 
    else if (p_evt_write->handle == select_device_handles.value_handle && p_evt_write->len == 2) {
        uint16_t selected_device_id = (p_evt_write->data[0] | (p_evt_write->data[1] << 8));
        NRF_LOG_INFO("✅ Selected Device ID: %d", selected_device_id);

        m_ant_device_id = selected_device_id;
        send_scan_result(m_ant_device_id, 0);

        save_device_config();  // This will reboot device
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
