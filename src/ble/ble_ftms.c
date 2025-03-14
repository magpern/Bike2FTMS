#include "ble_ftms.h"
#include "ble_srv_common.h"
#include "nrf_log.h"

/**@brief Function for adding the Indoor Bike Data Characteristic */
static uint32_t indoor_bike_data_char_add(ble_ftms_t * p_ftms) {
    ble_add_char_params_t add_char_params = {0};
    add_char_params.uuid              = BLE_UUID_INDOOR_BIKE_DATA_CHAR;
    add_char_params.uuid_type         = BLE_UUID_TYPE_BLE;
    add_char_params.init_len          = 9;   // Minimum size (speed, cadence)
    add_char_params.max_len           = 20;  // Max size (with optional fields)
    add_char_params.char_props.notify = 1;   
    add_char_params.is_var_len        = true;    
    add_char_params.cccd_write_access = SEC_OPEN;  

    NRF_LOG_INFO("Adding Indoor Bike Data Characteristic...");

    uint32_t err_code = characteristic_add(p_ftms->service_handle, &add_char_params, &p_ftms->indoor_bike_data_handles);
    if (err_code != NRF_SUCCESS) {
        NRF_LOG_ERROR("Failed to add Indoor Bike Data Char: 0x%08X", err_code);
    }
    return err_code;
}

/**@brief Function for adding the Training Status Characteristic */
static uint32_t training_status_char_add(ble_ftms_t * p_ftms) {
    ble_add_char_params_t add_char_params = {0};
    add_char_params.uuid              = BLE_UUID_TRAINING_STATUS_CHAR;
    add_char_params.uuid_type         = BLE_UUID_TYPE_BLE;
    add_char_params.init_len          = 1;   
    add_char_params.max_len           = 20;  
    add_char_params.char_props.notify = 1;    
    add_char_params.is_var_len        = true;
    add_char_params.cccd_write_access = SEC_OPEN;  

    NRF_LOG_INFO("Adding Training Status Characteristic...");

    uint32_t err_code = characteristic_add(p_ftms->service_handle, &add_char_params, &p_ftms->training_status_handles);
    if (err_code != NRF_SUCCESS) {
        NRF_LOG_ERROR("Failed to add Training Status Char: 0x%08X", err_code);
    }
    return err_code;
}

/**@brief Function for adding the Fitness Machine Status Characteristic */
static uint32_t machine_status_char_add(ble_ftms_t * p_ftms) {
    ble_add_char_params_t add_char_params = {0};
    add_char_params.uuid              = BLE_UUID_FITNESS_MACHINE_STATUS_CHAR;
    add_char_params.uuid_type         = BLE_UUID_TYPE_BLE;
    add_char_params.init_len          = 1;   
    add_char_params.max_len           = 3;   
    add_char_params.char_props.notify = 1;    
    add_char_params.is_var_len        = true;  
    add_char_params.cccd_write_access = SEC_OPEN;  

    NRF_LOG_INFO("Adding Fitness Machine Status Characteristic...");

    uint32_t err_code = characteristic_add(p_ftms->service_handle, &add_char_params, &p_ftms->fitness_machine_status_handles);
    if (err_code != NRF_SUCCESS) {
        NRF_LOG_ERROR("Failed to add Machine Status Char: 0x%08X", err_code);
    }
    return err_code;
}

/**@brief Function for adding the FTMS Feature characteristic */
uint32_t ble_ftms_feature_char_add(ble_ftms_t * p_ftms) {
    ble_add_char_params_t add_char_params = {0};
    static uint8_t featureData[8];  // 8-byte array for FTMS Feature Characteristic

    // ✅ Convert values to little-endian byte format
    uint32_t features = BLE_FTMS_FEATURES;
    uint32_t targetSettings = BLE_FTMS_TARGET_SETTINGS;
    memcpy(&featureData[0], &features, sizeof(features));
    memcpy(&featureData[4], &targetSettings, sizeof(targetSettings));

    add_char_params.uuid              = BLE_UUID_FTMS_FEATURE_CHAR;
    add_char_params.uuid_type         = BLE_UUID_TYPE_BLE;
    add_char_params.init_len          = sizeof(featureData);
    add_char_params.max_len           = sizeof(featureData);
    add_char_params.char_props.read   = 1;
    add_char_params.is_var_len        = false;
    add_char_params.read_access       = SEC_OPEN;
    add_char_params.p_init_value      = featureData;  // ✅ Store the feature bitmask

    NRF_LOG_INFO("Adding FTMS Feature Characteristic: UUID=0x%04X, Features=0x%08X, TargetSettings=0x%08X",
        BLE_UUID_FTMS_FEATURE_CHAR, features, targetSettings);

    return characteristic_add(p_ftms->service_handle, &add_char_params, &p_ftms->ftms_feature_handles);
}

/**@brief Function for initializing the FTMS service. */
uint32_t ble_ftms_init(ble_ftms_t * p_ftms) {
    uint32_t err_code;
    ble_uuid_t ble_uuid;

    // Assign service UUID
    ble_uuid.type = BLE_UUID_TYPE_BLE;
    ble_uuid.uuid = BLE_UUID_FTMS_SERVICE;

    // Add service to BLE stack
    NRF_LOG_INFO("Adding FTMS Service...");

    err_code = sd_ble_gatts_service_add(BLE_GATTS_SRVC_TYPE_PRIMARY, &ble_uuid, &p_ftms->service_handle);
    if (err_code != NRF_SUCCESS) {
        NRF_LOG_ERROR("FTMS Service add failed: 0x%08X", err_code);
        return err_code;
    }
    NRF_LOG_INFO("FTMS Service added successfully. Handle: %d", p_ftms->service_handle);
    
    // Add FTMS Characteristics
    err_code = indoor_bike_data_char_add(p_ftms);
    if (err_code != NRF_SUCCESS) return err_code;
    
    err_code = training_status_char_add(p_ftms);
    if (err_code != NRF_SUCCESS) return err_code;
    
    err_code = machine_status_char_add(p_ftms);
    if (err_code != NRF_SUCCESS) return err_code;
    
    err_code = ble_ftms_feature_char_add(p_ftms);
    if (err_code != NRF_SUCCESS) return err_code;

    return NRF_SUCCESS;
}

/**@brief Function to send FTMS Indoor Bike Data */
void ble_ftms_send_indoor_bike_data(ble_ftms_t * p_ftms, ble_ftms_data_t * p_data) {
    if (p_ftms->conn_handle == BLE_CONN_HANDLE_INVALID) return;

    uint8_t encoded_data[15] = {0};  // Ensure buffer is initialized to zero

    // ✅ 1. Correct Flag Bytes (0x74 0x08 matches the NimBLE implementation)
    encoded_data[0] = 0x74;  // Flag Byte 0: Speed, Cadence, Power, Resistance
    encoded_data[1] = 0x08;  // Flag Byte 1: Time Present

    // ✅ 2. Instantaneous Speed (Set to 0 km/h, since we don’t have speed data)
    encoded_data[2] = 0x00;
    encoded_data[3] = 0x00;

    // ✅ 3. Instantaneous Cadence (Multiply by 2 for FTMS resolution)
    uint16_t cadence = (uint16_t)(p_data->cadence_rpm * 2);
    encoded_data[4] = cadence & 0xFF;
    encoded_data[5] = (cadence >> 8) & 0xFF;

    // ✅ 4. Total Distance (Set to 0 since we don’t track it yet)
    encoded_data[6] = 0x00;
    encoded_data[7] = 0x00;
    encoded_data[8] = 0x00;

    // ✅ 5. Resistance Level (Set to 0, we don’t control resistance)
    encoded_data[9] = 0x00;
    encoded_data[10] = 0x00;

    // ✅ 6. Instantaneous Power (Signed 16-bit, Little-Endian)
    int16_t power = (int16_t)p_data->power_watts;
    encoded_data[11] = power & 0xFF;
    encoded_data[12] = (power >> 8) & 0xFF;

    // ✅ 7. Elapsed Time (Set to 0, we don't track elapsed time)
    encoded_data[13] = 0x00;
    encoded_data[14] = 0x00;

    // ✅ Send FTMS Data Over BLE
    ble_gatts_hvx_params_t hvx_params = {0};
    hvx_params.handle = p_ftms->indoor_bike_data_handles.value_handle;
    hvx_params.type   = BLE_GATT_HVX_NOTIFICATION;
    hvx_params.p_data = encoded_data;
    hvx_params.p_len  = (uint16_t[]){sizeof(encoded_data)};

    sd_ble_gatts_hvx(p_ftms->conn_handle, &hvx_params);
}


