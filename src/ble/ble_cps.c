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
    ble_add_char_params_t add_char_params = {0};

    add_char_params.uuid              = BLE_UUID_POWER_MEASUREMENT_CHAR;
    add_char_params.uuid_type         = BLE_UUID_TYPE_BLE;
    add_char_params.init_len          = 4;    // Minimum size (Flags + Instantaneous Power)
    add_char_params.max_len           = 4;   // Allow optional fields (Flags determine actual size)
    add_char_params.char_props.notify = 1;    // Enable notifications
    add_char_params.is_var_len        = true; // Allow variable-length messages

    // Enable CCCD for notifications
    add_char_params.cccd_write_access = SEC_OPEN;
    add_char_params.read_access       = SEC_OPEN;
    add_char_params.write_access      = SEC_NO_ACCESS;

    // ✅ Store default initial value (Power = 0W)
    static uint8_t initial_value[4] = {0x00, 0x00, 0x00, 0x00};  // Flags (2B) + Power (2B)

    add_char_params.p_init_value = initial_value;

    // ✅ Use `characteristic_add()` (More reliable than manual BLE GATTS API)
    return characteristic_add(p_cps->service_handle, &add_char_params, &p_cps->power_measurement_handles);
}

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
void ble_cps_send_power_measurement(ble_cps_t * p_cps, uint16_t power_watts) {
    if (p_cps->conn_handle == BLE_CONN_HANDLE_INVALID) {
        NRF_LOG_WARNING("⚠️ Invalid connection handle. Cannot send CPS notification.");
        return;
    }

    // Check if CCCD (Client Characteristic Configuration Descriptor) is enabled
    uint16_t cccd_value = 0;
    ble_gatts_value_t gatts_value = {
        .p_value = (uint8_t*)&cccd_value,
        .len = sizeof(cccd_value),
        .offset = 0
    };

    uint32_t err_code = sd_ble_gatts_value_get(
        p_cps->conn_handle, 
        p_cps->power_measurement_handles.cccd_handle, 
        &gatts_value
    );

    if (err_code != NRF_SUCCESS || cccd_value != BLE_GATT_HVX_NOTIFICATION) {
        //NRF_LOG_WARNING("⚠️ Notifications not enabled. Skipping CPS notification.");
        return;
    }

    // Prepare Cycling Power Measurement Data
    uint8_t encoded_data[4] = {0};  // 6 bytes: Flags (2B) + Instantaneous Power (2B) + Reserved (2B)

    // ✅ Flags (Set to 0, No optional fields used)
    encoded_data[0] = 0x00;
    encoded_data[1] = 0x00;

    // ✅ Instantaneous Power (Little-Endian, 2 bytes)
    encoded_data[2] = (power_watts & 0xFF);
    encoded_data[3] = (power_watts >> 8);


    // ✅ Log Debug Information
    NRF_LOG_INFO("🚴 Sending Power Measurement: %d W", power_watts);

    // ✅ Send BLE Notification
    ble_gatts_hvx_params_t hvx_params = {0};
    hvx_params.handle = p_cps->power_measurement_handles.value_handle;
    hvx_params.type   = BLE_GATT_HVX_NOTIFICATION;
    hvx_params.p_data = encoded_data;

    uint16_t len = sizeof(encoded_data);
    hvx_params.p_len = &len;  

    err_code = sd_ble_gatts_hvx(p_cps->conn_handle, &hvx_params);
    if (err_code != NRF_SUCCESS) {
        NRF_LOG_ERROR("❌ Failed to send CPS notification: 0x%08X", err_code);
    }
}






