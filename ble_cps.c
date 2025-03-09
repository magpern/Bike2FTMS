#include "ble_cps.h"
#include "nrf_log.h"
#include "app_error.h"

/**@brief Function for adding the Cycling Power Feature characteristic. */
static uint32_t power_feature_char_add(ble_cps_t * p_cps) {
    ble_add_char_params_t add_char_params = {0};

    add_char_params.uuid      = BLE_UUID_CYCLING_POWER_FEATURE_CHAR;
    add_char_params.uuid_type = BLE_UUID_TYPE_BLE;
    add_char_params.init_len  = 4;  // **Fixed 4-byte feature characteristic**
    add_char_params.max_len   = 4;  // **Fixed length**
    add_char_params.is_var_len = false;  // **Fixed length characteristic**
    add_char_params.char_props.read = 1;  // **Read-only characteristic**
    add_char_params.read_access = SEC_OPEN;  // **Ensure open access**

    // **No features, exept mandatory instantanious power
    static uint8_t feature_value[4] = {0x00, 0x00, 0x00, 0x00};

    add_char_params.p_init_value = feature_value;

    return characteristic_add(p_cps->service_handle, &add_char_params, &p_cps->feature_handles);
}


/**@brief Function for adding the Cycling Power Measurement characteristic. */
static uint32_t power_measurement_char_add(ble_cps_t * p_cps) {
    ble_gatts_char_md_t char_md = {0};
    ble_gatts_attr_md_t cccd_md = {0};
    ble_gatts_attr_md_t attr_md = {0};
    ble_gatts_attr_t attr_char_value = {0};
    ble_uuid_t ble_uuid;

    // Set characteristic UUID
    ble_uuid.type = BLE_UUID_TYPE_BLE;
    ble_uuid.uuid = BLE_UUID_POWER_MEASUREMENT_CHAR;

    // Enable notifications
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&cccd_md.read_perm);
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&cccd_md.write_perm);
    cccd_md.vloc = BLE_GATTS_VLOC_STACK;

    char_md.char_props.notify = 1;
    char_md.p_cccd_md = &cccd_md;

    // Set attribute metadata
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&attr_md.read_perm);
    BLE_GAP_CONN_SEC_MODE_SET_NO_ACCESS(&attr_md.write_perm);
    attr_md.vloc = BLE_GATTS_VLOC_STACK;

    // Corrected initial value (14 bytes for full spec)
    static uint8_t initial_value[4] = {0};

    // Assign attribute
    attr_char_value.p_uuid = &ble_uuid;
    attr_char_value.p_attr_md = &attr_md;
    attr_char_value.init_len = sizeof(initial_value); // 14 bytes now
    attr_char_value.max_len  = sizeof(initial_value); // Maximum allowed size
    attr_char_value.p_value  = initial_value;
    attr_char_value.init_offs = 0;
    
    return sd_ble_gatts_characteristic_add(p_cps->service_handle, &char_md, &attr_char_value, &p_cps->power_measurement_handles);
}



/**@brief Function for adding the Sensor Location characteristic. */
/**@brief Function for adding the Sensor Location characteristic. */
static uint32_t sensor_location_char_add(ble_cps_t * p_cps) {
    ble_add_char_params_t add_char_params = {0};

    add_char_params.uuid      = BLE_UUID_SENSOR_LOCATION_CHAR;
    add_char_params.uuid_type = BLE_UUID_TYPE_BLE;
    add_char_params.init_len  = 1;
    add_char_params.max_len   = 1;
    add_char_params.char_props.read = 1;  // Read-only characteristic
    add_char_params.read_access = SEC_OPEN;  // Open access

    // Default sensor location (e.g., Right Crank = 0x06)
    static uint8_t sensor_location_value = 0x00; //Other

    add_char_params.p_init_value = &sensor_location_value;
    
    return characteristic_add(p_cps->service_handle, &add_char_params, &p_cps->sensor_location_handles);
}


/**@brief Function for initializing the Cycling Power Service. */
uint32_t ble_cps_init(ble_cps_t * p_cps) {
    uint32_t err_code;
    ble_uuid_t ble_uuid;

    // Assign Cycling Power Service UUID (0x1818)
    ble_uuid.type = BLE_UUID_TYPE_BLE;
    ble_uuid.uuid = BLE_UUID_CYCLING_POWER_SERVICE;

    // Add CPS service to BLE stack
    NRF_LOG_INFO("Adding Cycling Power Service...");

    err_code = sd_ble_gatts_service_add(BLE_GATTS_SRVC_TYPE_PRIMARY, &ble_uuid, &p_cps->service_handle);
    if (err_code != NRF_SUCCESS) {
        NRF_LOG_ERROR("Cycling Power Service add failed: 0x%08X", err_code);
        return err_code;
    }
    NRF_LOG_INFO("Cycling Power Service added successfully. Handle: %d", p_cps->service_handle);
    
    // Add CPS Characteristics
    err_code = power_measurement_char_add(p_cps);
    if (err_code != NRF_SUCCESS) return err_code;

    err_code = power_feature_char_add(p_cps);
    if (err_code != NRF_SUCCESS) return err_code;

    err_code = sensor_location_char_add(p_cps);
    if (err_code != NRF_SUCCESS) return err_code;

    return NRF_SUCCESS;
}

/**@brief Function for sending Cycling Power Measurement notifications. */
/**@brief Function for sending Cycling Power Measurement notifications. */
void ble_cps_send_power_measurement(ble_cps_t * p_cps, uint16_t power_watts) {
    if (p_cps->conn_handle == BLE_CONN_HANDLE_INVALID) return;

    uint8_t encoded_data[4] = {0};  // 4 bytes: Flags (2B) + Power (2B)

    // ✅ 1. Flags (No optional fields)
    encoded_data[0] = 0x00;  // No pedal balance, torque, cadence, or revolutions
    encoded_data[1] = 0x00;  // Flags are little-endian (LSB first)

    // ✅ 2. Instantaneous Power (Always included, 2 bytes, Little-Endian)
    encoded_data[2] = power_watts & 0xFF;
    encoded_data[3] = (power_watts >> 8);

    // ✅ Debug Logging
    NRF_LOG_INFO("Sending Power Measurement: %d W", power_watts);

    // ✅ Send BLE Notification
    ble_gatts_hvx_params_t hvx_params = {0};
    hvx_params.handle = p_cps->power_measurement_handles.value_handle;
    hvx_params.type   = BLE_GATT_HVX_NOTIFICATION;
    hvx_params.p_data = encoded_data;

    uint16_t len = sizeof(encoded_data);
    hvx_params.p_len = &len;  

    uint32_t err_code = sd_ble_gatts_hvx(p_cps->conn_handle, &hvx_params);
    if (err_code != NRF_SUCCESS) {
        NRF_LOG_ERROR("Failed to send CPS notification: 0x%08X", err_code);
    }
}





