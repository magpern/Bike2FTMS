#include "ble.h"
#include "ble_srv_common.h"
#include "fds.h"
#include "nrf_log.h"
#include "app_error.h"
#include "ble_custom_config.h"
#include "nrf_delay.h"
#include "nrf_pwr_mgmt.h"

#define CUSTOM_SERVICE_UUID          0x1523
#define CUSTOM_CHAR_DEVICE_INFO_UUID 0x1524  

#define DEFAULT_BLE_NAME       "SatsBike"
#define BLE_NAME_MAX_LEN       8
#define DEVICE_INFO_LEN        (2 + 1 + BLE_NAME_MAX_LEN)  
#define DEFAULT_ANT_DEVICE_ID  18465  

static uint16_t m_service_handle;
static ble_gatts_char_handles_t m_device_info_handles;

static uint8_t device_info[DEVICE_INFO_LEN];  

// ✅ Global variables for access throughout the program
uint16_t m_ant_device_id = DEFAULT_ANT_DEVICE_ID;
char m_ble_name[BLE_NAME_MAX_LEN + 1] = DEFAULT_BLE_NAME;  // ✅ Null-terminated name
// ✅ Define `ble_full_name` here (it was only declared in the .h file)
char ble_full_name[MAX_BLE_FULL_NAME_LEN] = {0};

void update_ble_name() {
    // ✅ Use the global `ble_full_name` instead of a local variable
    snprintf(ble_full_name, sizeof(ble_full_name), "%.*s_%05d",
             BLE_NAME_MAX_LEN, m_ble_name, m_ant_device_id);
    
    NRF_LOG_INFO("Updated BLE Full Name: %s", ble_full_name);
}

/**@brief Function for handling BLE writes. */
static void on_write(ble_evt_t const *p_ble_evt) {
    ble_gatts_evt_write_t const *p_evt_write = &p_ble_evt->evt.gatts_evt.params.write;
    ret_code_t err_code;

    if (p_evt_write->handle == m_device_info_handles.value_handle) {
        uint16_t write_len = p_evt_write->len;

        if (write_len < 3 || write_len > DEVICE_INFO_LEN) {
            NRF_LOG_WARNING("Invalid Data Length: %d bytes", write_len);
            return;
        }

        memset(device_info, 0, DEVICE_INFO_LEN);
        memcpy(device_info, p_evt_write->data, write_len);

        // ✅ Update global Device ID
        m_ant_device_id = (uint16_t)(device_info[0] | (device_info[1] << 8));

        // ✅ Extract BLE Name Length
        uint8_t name_length = device_info[2];
        if (name_length > BLE_NAME_MAX_LEN) name_length = BLE_NAME_MAX_LEN;

        // ✅ Store BLE Name in global variable (m_ble_name)
        memset(m_ble_name, 0, BLE_NAME_MAX_LEN + 1);
        uint8_t valid_length = 0;
        for (uint8_t i = 0; i < name_length; i++) {
            char c = device_info[3 + i];
            if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
                (c >= '0' && c <= '9') || (c == '_')) {
                m_ble_name[valid_length++] = c;
            }
        }
        m_ble_name[valid_length] = '\0';  // ✅ Ensure null termination

        fds_gc();
        while (fds_gc() == FDS_ERR_BUSY) {
            nrf_delay_ms(50);
        }

        NRF_LOG_INFO("New Device ID: %d", m_ant_device_id);
        NRF_LOG_INFO("New BLE Name: %s", m_ble_name);

        // ✅ Store in Flash
        fds_record_t record = {
            .file_id = 0xBEEF,
            .key = 0x0001,
            .data.p_data = device_info,
            .data.length_words = (DEVICE_INFO_LEN + 3) / 4 + ((DEVICE_INFO_LEN % 4) ? 1 : 0)
        };

        fds_record_desc_t desc;  // ✅ No need for `ftok` here
        err_code = fds_record_write(&desc, &record);
        if (err_code == NRF_SUCCESS) {
            NRF_LOG_INFO("✅ Device Info successfully written to Flash.");
            
            fds_record_close(&desc);  // ✅ Explicitly close record
            NRF_LOG_INFO("✅ Flash Write Complete and Closed.");

            // ✅ Ensure Flash Write Completes Before Rebooting
            while (fds_gc() == FDS_ERR_BUSY) {
                nrf_delay_ms(50);
            }
            
            NRF_LOG_INFO("Flash Write Complete. Rebooting...");
            nrf_delay_ms(500);
            NVIC_SystemReset();
        } else {
            NRF_LOG_ERROR("❌ Flash write failed! Error Code: %d", err_code);
        }
    }
}



/**@brief Function for handling BLE events (write only). */
void ble_custom_service_on_ble_evt(ble_evt_t const *p_ble_evt, void *p_context) {
    if (p_ble_evt->header.evt_id == BLE_GATTS_EVT_WRITE) {
        on_write(p_ble_evt);
    }
}

/**@brief Function for initializing the custom BLE service. */
void ble_custom_service_init(void) {
    ble_uuid_t ble_uuid = {.uuid = CUSTOM_SERVICE_UUID, .type = BLE_UUID_TYPE_BLE};
    sd_ble_gatts_service_add(BLE_GATTS_SRVC_TYPE_PRIMARY, &ble_uuid, &m_service_handle);

    ble_add_char_params_t add_char_params = {0};

    add_char_params.uuid = CUSTOM_CHAR_DEVICE_INFO_UUID;
    add_char_params.uuid_type = BLE_UUID_TYPE_BLE;
    add_char_params.init_len = DEVICE_INFO_LEN;
    add_char_params.max_len = DEVICE_INFO_LEN;
    add_char_params.p_init_value = device_info;
    add_char_params.char_props.read = 1;
    add_char_params.char_props.write = 1;
    add_char_params.read_access = SEC_OPEN;
    add_char_params.write_access = SEC_OPEN;
    characteristic_add(m_service_handle, &add_char_params, &m_device_info_handles);
}

/**@brief Function to load stored values from flash memory. */
void custom_service_load_from_flash(void) {
    ret_code_t err_code;
    fds_flash_record_t flash_record;
    fds_find_token_t ftok = {0};  // ✅ Correctly initialize search token
    fds_record_desc_t desc;

    NRF_LOG_INFO("Initializing Flash Data Storage...");
    err_code = fds_init();
    APP_ERROR_CHECK(err_code);
    nrf_delay_ms(100);

    ret_code_t find_err = fds_record_find(0xBEEF, 0x0001, &desc, &ftok);
    NRF_LOG_INFO("FDS Record Find Status: %d", find_err);
    if (find_err == NRF_SUCCESS) {
        err_code = fds_record_open(&desc, &flash_record);
        if (err_code == NRF_SUCCESS && flash_record.p_data != NULL) {
            memset(device_info, 0, DEVICE_INFO_LEN);
            memcpy(device_info, flash_record.p_data, DEVICE_INFO_LEN);
            fds_record_close(&desc);

            // ✅ Extract Device ID
            m_ant_device_id = (uint16_t)(device_info[0] | (device_info[1] << 8));

            // ✅ Extract BLE Name Length
            uint8_t stored_length = device_info[2];
            if (stored_length > BLE_NAME_MAX_LEN) stored_length = BLE_NAME_MAX_LEN;

            // ✅ Store BLE Name in global variable (m_ble_name)
            memset(m_ble_name, 0, BLE_NAME_MAX_LEN + 1);
            uint8_t valid_length = 0;
            for (uint8_t i = 0; i < stored_length; i++) {
                char c = device_info[3 + i];
                if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
                    (c >= '0' && c <= '9') || (c == '_')) {
                    m_ble_name[valid_length++] = c;
                }
            }
            m_ble_name[valid_length] = '\0';  // ✅ Ensure null termination

            NRF_LOG_INFO("Loaded Device ID: %d", m_ant_device_id);
            NRF_LOG_INFO("Loaded BLE Name: %s", m_ble_name);
        } else {
            NRF_LOG_WARNING("No valid stored Device Info found.");
            goto load_defaults;
        }
    } else {
        NRF_LOG_WARNING("No stored Device Info found.");
        goto load_defaults;
    }

    update_ble_name();
    return;

load_defaults:
    m_ant_device_id = DEFAULT_ANT_DEVICE_ID;
    strncpy(m_ble_name, DEFAULT_BLE_NAME, BLE_NAME_MAX_LEN);
    m_ble_name[BLE_NAME_MAX_LEN] = '\0';  

    NRF_LOG_WARNING("Using defaults: Device ID = %d, BLE Name = %s", m_ant_device_id, m_ble_name);
    update_ble_name();
}


