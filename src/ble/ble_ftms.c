#include "ble_ftms.h"
#include "ble_srv_common.h"
#include "nrf_log.h"
#include <common_definitions.h>
#include "app_timer.h"  // Required for app_timer

APP_TIMER_DEF(ftms_training_timer);  // Timer instance

typedef enum {
    TRAINING_STATUS_IDLE           = 0x01,
    TRAINING_STATUS_WARMING_UP     = 0x02,
    TRAINING_STATUS_LOW_INTENSITY  = 0x03,
    TRAINING_STATUS_ACTIVE         = 0x04,
    TRAINING_STATUS_RECOVERY       = 0x05,
    TRAINING_STATUS_PAUSED         = 0x06,
    TRAINING_STATUS_FINISHED       = 0x07,
    TRAINING_STATUS_UNKNOWN        = 0x08,
    TRAINING_STATUS_ABORTED        = 0x09
} training_state_t;

#define FTMS_INACTIVITY_TIMEOUT_MS 5000  // 5 seconds
#define FTMS_MAX_PAUSE_COUNT 1           // After this, transition to FINISHED

static training_state_t current_training_state = TRAINING_STATUS_IDLE;
static uint8_t status_repeat_counter = 0;  // Avoid repeating notifications unnecessarily

static void ftms_training_timer_handler(void *p_context) {
    if (current_training_state == TRAINING_STATUS_ACTIVE) {
        current_training_state = TRAINING_STATUS_PAUSED;
        NRF_LOG_INFO("⏸️ FTMS status changed to PAUSED");
        status_repeat_counter = 0;
    } else if (current_training_state == TRAINING_STATUS_PAUSED) {
        current_training_state = TRAINING_STATUS_FINISHED;
        NRF_LOG_INFO("🛑 FTMS status changed to FINISHED");
        status_repeat_counter = 0;
    }

    // If already FINISHED, don't change anything further
}

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
    add_char_params.init_len          = 2;   // Flags (1) + Status (1)
    add_char_params.max_len           = 20;  // Allow room for optional UTF-8 string
    add_char_params.char_props.notify = 1;
    add_char_params.char_props.read   = 1;   // ✅ FTMS spec requires READ
    add_char_params.is_var_len        = true;
    add_char_params.read_access       = SEC_OPEN;
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
    ret_code_t err_code;
    ble_uuid_t ble_uuid;

    // Assign service UUID
    ble_uuid.type = BLE_UUID_TYPE_BLE;
    ble_uuid.uuid = BLE_UUID_FTMS_SERVICE;
    p_ftms->conn_handle = BLE_CONN_HANDLE_INVALID;

    // Add service to BLE stack
    NRF_LOG_INFO("Adding FTMS Service...");

    err_code = sd_ble_gatts_service_add(BLE_GATTS_SRVC_TYPE_PRIMARY, &ble_uuid, &p_ftms->service_handle);
    if (err_code != NRF_SUCCESS) {
        NRF_LOG_ERROR("FTMS Service add failed: 0x%08X", err_code);
        return err_code;
    }
    NRF_LOG_INFO("FTMS Service added successfully. Handle: %d", p_ftms->service_handle);
    
    err_code = app_timer_create(
        &ftms_training_timer,
        APP_TIMER_MODE_SINGLE_SHOT,
        ftms_training_timer_handler
    );
    APP_ERROR_CHECK(err_code);
    
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




static int16_t last_power = -1;
static uint8_t last_cadence = 0xFF;
static uint8_t _duplicate_counter = 0;

static void _ble_ftms_send_indoor_bike_data(ble_ftms_t * p_ftms, ble_ftms_data_t * p_data) {
    if (p_ftms->conn_handle == BLE_CONN_HANDLE_INVALID) return;

    // Check if CCCD (Client Characteristic Configuration Descriptor) is enabled
    uint16_t cccd_value = 0;
    ble_gatts_value_t gatts_value = {
        .p_value = (uint8_t*)&cccd_value,
        .len = sizeof(cccd_value),
        .offset = 0
    };

    uint32_t err_code = sd_ble_gatts_value_get(
        p_ftms->conn_handle,
        p_ftms->indoor_bike_data_handles.cccd_handle,  // Should be: indoor_bike_data_handles.cccd_handle
        &gatts_value
    );

    if (err_code != NRF_SUCCESS || (cccd_value & BLE_GATT_HVX_NOTIFICATION) == 0) {
        NRF_LOG_WARNING("⚠️ FTMS notifications not enabled. Skipping.");
        return;
    }

    // 🧠 Deduplication logic
    if (p_data->power_watts == last_power && p_data->cadence_rpm == last_cadence) {
        _duplicate_counter++;
        if (_duplicate_counter < RESET_DUPLICATE_COUNTER_EVERY_N_MESSAGE) {
            NRF_LOG_WARNING("⏩ FTMS duplicate (P:%d W, C:%d RPM), skipping [%d/%d]",
                          p_data->power_watts, p_data->cadence_rpm,
                          _duplicate_counter, RESET_DUPLICATE_COUNTER_EVERY_N_MESSAGE);
            return;
        } else {
            NRF_LOG_INFO("🔁 FTMS duplicate threshold reached. Forcing update (P:%d W, C:%d RPM)",
                          p_data->power_watts, p_data->cadence_rpm);
            _duplicate_counter = 0;  // Reset after forced send
        }
    } else {
        _duplicate_counter = 0;  // Reset if values changed
    }

    // Prepare FTMS data packet
    uint8_t encoded_data[15] = {0};

    encoded_data[0] = 0x74;  // Flags
    encoded_data[1] = 0x08;

    encoded_data[2] = 0x00;  // Speed
    encoded_data[3] = 0x00;

    uint16_t cadence = (uint16_t)(p_data->cadence_rpm * 2);
    encoded_data[4] = cadence & 0xFF;
    encoded_data[5] = (cadence >> 8) & 0xFF;

    encoded_data[6] = 0x00;  // Distance
    encoded_data[7] = 0x00;
    encoded_data[8] = 0x00;

    encoded_data[9]  = 0x00;  // Resistance Level
    encoded_data[10] = 0x00;

    int16_t power = (int16_t)p_data->power_watts;
    encoded_data[11] = power & 0xFF;
    encoded_data[12] = (power >> 8) & 0xFF;

    encoded_data[13] = 0x00;  // Elapsed Time
    encoded_data[14] = 0x00;

    ble_gatts_hvx_params_t hvx_params = {0};
    hvx_params.handle = p_ftms->indoor_bike_data_handles.value_handle;
    hvx_params.type   = BLE_GATT_HVX_NOTIFICATION;
    hvx_params.p_data = encoded_data;

    uint16_t len = sizeof(encoded_data);
    hvx_params.p_len = &len;

    err_code = sd_ble_gatts_hvx(p_ftms->conn_handle, &hvx_params);
    if (err_code != NRF_SUCCESS) {
        NRF_LOG_ERROR("❌ Failed to send FTMS notification: 0x%08X", err_code);
    } else {
        NRF_LOG_INFO("🚴 FTMS Power Sent: %d W, Cadence: %d RPM", p_data->power_watts, p_data->cadence_rpm);
        last_power = p_data->power_watts;
        last_cadence = p_data->cadence_rpm;
    }
}

void ble_ftms_tick(ble_ftms_t *p_ftms, uint16_t power_watts, uint8_t cadence_rpm) {
    if (p_ftms->conn_handle == BLE_CONN_HANDLE_INVALID)
        return;

    // 1. Update training status
    bool is_active = (power_watts > 0 || cadence_rpm > 0);

    if (is_active) {
        if (current_training_state != TRAINING_STATUS_ACTIVE) {
            current_training_state = TRAINING_STATUS_ACTIVE;
            NRF_LOG_INFO("🏃 FTMS status changed to TRAINING");
            status_repeat_counter = 0;
        }

        // Reset inactivity timer
        app_timer_stop(ftms_training_timer);
        app_timer_start(ftms_training_timer, APP_TIMER_TICKS(FTMS_INACTIVITY_TIMEOUT_MS), NULL);
    }

    // 2. Notify Training Status if changed or periodically
    if (p_ftms->training_status_handles.cccd_handle != BLE_GATT_HANDLE_INVALID) {
        static training_state_t last_sent_status = TRAINING_STATUS_IDLE;

        if (current_training_state != last_sent_status || status_repeat_counter == 0) {
            uint8_t payload[2];  // Minimum required: Flags + Status
            payload[0] = 0x00;   // Flags: No optional string
            payload[1] = current_training_state;  // Training Status byte

            ble_gatts_hvx_params_t hvx_params = {0};
            hvx_params.handle = p_ftms->training_status_handles.value_handle;
            hvx_params.type = BLE_GATT_HVX_NOTIFICATION;
            hvx_params.p_data = payload;

            uint16_t len = sizeof(payload);
            hvx_params.p_len = &len;

            uint16_t cccd_value = 0;
            ble_gatts_value_t cccd_val = {
                .p_value = (uint8_t *)&cccd_value,
                .len = sizeof(cccd_value),
                .offset = 0
            };

            if (sd_ble_gatts_value_get(p_ftms->conn_handle, p_ftms->training_status_handles.cccd_handle, &cccd_val) == NRF_SUCCESS
                && (cccd_value & BLE_GATT_HVX_NOTIFICATION)) {
                sd_ble_gatts_hvx(p_ftms->conn_handle, &hvx_params);
                NRF_LOG_INFO("📢 Sent Training Status: Flags=0x%02X Status=0x%02X", payload[0], payload[1]);
                last_sent_status = current_training_state;
                status_repeat_counter++;
            }
        }
    }

    // 3. Call Indoor Bike Data sender (reuse your existing deduped logic)
    ble_ftms_data_t ftms_data = {
        .power_watts = power_watts,
        .cadence_rpm = cadence_rpm
    };
    _ble_ftms_send_indoor_bike_data(p_ftms, &ftms_data);
}


