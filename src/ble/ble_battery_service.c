#include "ble_battery_service.h"
#include "ble_srv_common.h"
#include "nrf_log.h"
#include "app_error.h"
#include "nrf_sdh_ble.h"
#include "ble_setup.h"  // ✅ Include to access `m_conn_handle`
#include "common_definitions.h"
#include "battery_measurement.h"
#include <stdlib.h>  // ✅ Required for `rand()`

ble_battery_t m_battery_service;
static uint8_t last_battery_level = 255;  // Stores the last sent battery level (initialize with invalid value)
static uint16_t last_voltage_mv = 0;  // Stores the last sent voltage

/**@brief Function to add a characteristic */
static uint32_t battery_char_add(uint16_t uuid, ble_gatts_char_handles_t *p_handles, uint8_t max_len) {
    ble_add_char_params_t char_params = {0};
    char_params.uuid = uuid;
    char_params.uuid_type = BLE_UUID_TYPE_BLE;
    char_params.init_len = 1;
    char_params.max_len = max_len;
    char_params.char_props.read = 1;
    char_params.char_props.notify = 1;
    char_params.cccd_write_access = SEC_OPEN;
    char_params.read_access = SEC_OPEN;

    return characteristic_add(m_battery_service.service_handle, &char_params, p_handles);
}

/**@brief Function to initialize the Battery Service */
void ble_battery_service_init(void) {
    ble_uuid_t ble_uuid;
    ble_uuid.type = BLE_UUID_TYPE_BLE;
    ble_uuid.uuid = BATTERY_SERVICE_UUID;
    m_battery_service.conn_handle = BLE_CONN_HANDLE_INVALID;

    sd_ble_gatts_service_add(BLE_GATTS_SRVC_TYPE_PRIMARY, &ble_uuid, &m_battery_service.service_handle);

    // ✅ Add Battery Level Characteristic (0x2A19)
    battery_char_add(BATTERY_LEVEL_CHAR_UUID, &m_battery_service.battery_level_handles, 1);

    // ✅ Add Battery Power State Characteristic (0x2A1A)
    battery_char_add(BATTERY_POWER_STATE_CHAR_UUID, &m_battery_service.battery_power_state_handles, 1);

    NRF_LOG_INFO("✅ Battery Service Initialized with Battery Level & Power State");

    // ✅ Initialize Battery Level to 100%
    uint8_t initial_battery_level = 100;
    ble_gatts_value_t battery_level_value = {
        .len = sizeof(initial_battery_level),
        .offset = 0,
        .p_value = &initial_battery_level
    };

    sd_ble_gatts_value_set(
        BLE_CONN_HANDLE_INVALID, 
        m_battery_service.battery_level_handles.value_handle, 
        &battery_level_value);


        // ✅ Initialize Battery Power State to "GOOD"
        uint8_t initial_power_state = 0;
        initial_power_state |= (3 << 0); // Present
        initial_power_state |= (2 << 2); // Not Discharging
        initial_power_state |= (2 << 6); // Good Level

        ble_gatts_value_t power_state_value = {
            .len = sizeof(initial_power_state),
            .offset = 0,
            .p_value = &initial_power_state
        };
        sd_ble_gatts_value_set(
            BLE_CONN_HANDLE_INVALID, 
            m_battery_service.battery_power_state_handles.value_handle, 
            &power_state_value);


    NRF_LOG_INFO("🔋 Battery Level set to 100%%, Power State set to GOOD on boot");
}


/**@brief Function to send battery level updates */
void ble_battery_update(uint8_t battery_level, uint16_t voltage_mv, uint8_t power_state) {
    if (m_battery_service.conn_handle == BLE_CONN_HANDLE_INVALID) return;

    // ✅ Prevent redundant updates
    if (battery_level == last_battery_level && voltage_mv == last_voltage_mv) {
        NRF_LOG_INFO("🔋 No change in battery level or voltage, skipping update.");
        return;
    }

    last_battery_level = battery_level;
    last_voltage_mv = voltage_mv;

    ret_code_t err_code;
    ble_gatts_value_t gatts_value;
    ble_gatts_hvx_params_t hvx_params = {0};

    // ✅ Update Battery Level (0x2A19)
    gatts_value.len = sizeof(battery_level);
    gatts_value.offset = 0;
    gatts_value.p_value = &battery_level;
    err_code = sd_ble_gatts_value_set(m_battery_service.conn_handle, m_battery_service.battery_level_handles.value_handle, &gatts_value);
    APP_ERROR_CHECK(err_code);

    // ✅ Check if notifications are enabled before sending
    uint16_t cccd_value = 0;
    ble_gatts_value_t cccd_val = {.len = sizeof(cccd_value), .p_value = (uint8_t *)&cccd_value};
    err_code = sd_ble_gatts_value_get(m_battery_service.conn_handle, m_battery_service.battery_level_handles.cccd_handle, &cccd_val);

    if (err_code == NRF_SUCCESS && (cccd_value & BLE_GATT_HVX_NOTIFICATION)) {
        hvx_params.handle = m_battery_service.battery_level_handles.value_handle;
        hvx_params.type = BLE_GATT_HVX_NOTIFICATION;
        hvx_params.p_data = &battery_level;
        hvx_params.p_len = &gatts_value.len;
        sd_ble_gatts_hvx(m_battery_service.conn_handle, &hvx_params);
    }

    // ✅ Update Battery Power State (0x2A1A)
    gatts_value.len = sizeof(power_state);
    gatts_value.p_value = &power_state;
    err_code = sd_ble_gatts_value_set(m_battery_service.conn_handle, m_battery_service.battery_power_state_handles.value_handle, &gatts_value);
    APP_ERROR_CHECK(err_code);

    // ✅ Send Battery Power State notification
    err_code = sd_ble_gatts_value_get(m_battery_service.conn_handle, m_battery_service.battery_power_state_handles.cccd_handle, &cccd_val);
    if (err_code == NRF_SUCCESS && (cccd_value & BLE_GATT_HVX_NOTIFICATION)) {
        hvx_params.handle = m_battery_service.battery_power_state_handles.value_handle;
        hvx_params.p_data = &power_state;
        hvx_params.p_len = &gatts_value.len;
        sd_ble_gatts_hvx(m_battery_service.conn_handle, &hvx_params);
    }

    NRF_LOG_INFO("🔋 Sent Battery Level: %d%%, Voltage: %dmV, Power State: 0x%02X", battery_level, voltage_mv, power_state);
}

/**@brief Update Battery Level and Notify BLE */
void update_battery(void *p_context) {
    if (m_battery_service.conn_handle == BLE_CONN_HANDLE_INVALID) {
        NRF_LOG_INFO("⚠️ No BLE connection, skipping battery update.");
        return;
    }
    
    static uint8_t last_battery_percent = 0xFF; // Invalid initial value
    static uint16_t last_voltage_mv = 0xFFFF;   // Invalid initial value

    uint8_t battery_percent = battery_level_get();  // ✅ Get percentage (0-100)
    //battery_percent = (rand() % 101);
    uint16_t voltage_mv = get_battery_voltage();   // ✅ Get last measured voltage (mV)

    // ✅ Calculate Battery Power State (0x2A1A)
    uint8_t battery_power_state = 0;
    battery_power_state |= (3 << 0);  // Present (Bits 0-1)
    battery_power_state |= (2 << 2);  // Not Discharging (Bits 2-3)
    battery_power_state |= (battery_percent < 10) ? (3 << 6) : (2 << 6);  // Critically Low or Good Level

    // ✅ Always send update when a BLE device connects
    if (battery_percent == last_battery_percent && voltage_mv == last_voltage_mv) {
        NRF_LOG_INFO("🔋 No change in battery level or voltage, skipping update.");
        return;
    }
    ble_battery_update(battery_percent, voltage_mv, battery_power_state);
    last_battery_percent = battery_percent;
    last_voltage_mv = voltage_mv;
    NRF_LOG_INFO("🔋 Battery Updated: %d%% (%dmV), Power State: 0x%02X", battery_percent, voltage_mv, battery_power_state);
}
