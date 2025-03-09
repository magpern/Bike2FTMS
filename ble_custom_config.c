#include "ble.h"
#include "ble_srv_common.h"
#include "fds.h"
#include "nrf_log.h"
#include "app_error.h"
#include "ble_custom_config.h"
#include "nrf_delay.h"
#include "nrf_pwr_mgmt.h"  // ✅ Required for system reboot

#define CUSTOM_SERVICE_UUID           0x1523
#define CUSTOM_CHAR_ANT_DEVICE_ID_UUID 0x1524
#define CUSTOM_CHAR_BLE_NAME_UUID      0x1525

#define DEFAULT_BLE_NAME      "SatsBike"
#define BLE_NAME_MAX_ADVERTISED 15  // Safe max length for advertising
#define MAX_CUSTOM_NAME_LEN 8  // ✅ Limit user-defined name to 8 characters
#define DEVICE_ID_MAX_LEN 5         // ANT+ Device ID is 5 digits
#define DEFAULT_ANT_DEVICE_ID 18465 // Default ANT+ Device ID
static uint16_t m_conn_handle = BLE_CONN_HANDLE_INVALID;
static uint16_t m_service_handle;
static ble_gatts_char_handles_t m_ant_device_id_handles;
static ble_gatts_char_handles_t m_ble_name_handles;
static char m_ble_name[BLE_NAME_MAX_ADVERTISED ];  // ✅ Now local, no longer global
char ble_full_name[MAX_BLE_FULL_NAME_LEN]; // ✅ Now globally accessible
uint16_t m_ant_device_id = DEFAULT_ANT_DEVICE_ID;  // ✅ Global definition

void update_ble_name() {
    memset(ble_full_name, 0, MAX_BLE_FULL_NAME_LEN);

    // ✅ Format: "<CustomName>_<DeviceID>", ensuring it fits 14 characters
    snprintf(ble_full_name, MAX_BLE_FULL_NAME_LEN, "%.*s_%05d", 
             MAX_CUSTOM_NAME_LEN,  // ✅ Limit user-defined name to 8 characters
             m_ble_name, m_ant_device_id);
}

/**@brief Function for handling BLE writes to characteristics. */
static void on_write(ble_evt_t const * p_ble_evt) {
    ble_gatts_evt_write_t const * p_evt_write = &p_ble_evt->evt.gatts_evt.params.write;
    fds_record_desc_t desc;
    fds_find_token_t ftok = {0};  // Reset find token for searches
    ret_code_t err_code;

    // ✅ Check available FDS space before writing
    fds_stat_t stat;
    fds_stat(&stat);  // ✅ Correctly calling the function and storing in `stat`   
    NRF_LOG_INFO("FDS Space - Records: %d, Words Used: %d, Words Free: %d",
        stat.valid_records, stat.words_used, stat.freeable_words);

    if (p_evt_write->handle == m_ant_device_id_handles.value_handle) {
        if (p_evt_write->len == sizeof(m_ant_device_id)) {
            memcpy(&m_ant_device_id, p_evt_write->data, sizeof(m_ant_device_id));

            NRF_LOG_INFO("Deleting old Device ID before writing new one...");

            // ✅ Delete existing record if found
            if (fds_record_find(0xBEEF, 0x0001, &desc, &ftok) == NRF_SUCCESS) {
                fds_record_delete(&desc);
                fds_gc();  // ✅ Force garbage collection before writing
                NRF_LOG_INFO("Old Device ID deleted.");
            }

            // ✅ Delay to ensure garbage collection completes
            nrf_delay_ms(100);

            NRF_LOG_INFO("Saving new Device ID to Flash: %d", m_ant_device_id);

            fds_record_t record = {
                .file_id = 0xBEEF,
                .key = 0x0001,
                .data.p_data = &m_ant_device_id,
                .data.length_words = 1
            };

            err_code = fds_record_write(NULL, &record);
            if (err_code == NRF_SUCCESS) {
                NRF_LOG_INFO("✅ Device ID successfully written to Flash.");
                // ✅ Reboot the device to apply the new BLE name
                NRF_LOG_INFO("Rebooting device to apply new BLE name...");
                nrf_delay_ms(500);  // ✅ Short delay before reset
                NVIC_SystemReset();  // ✅ Trigger a system reset
            } else {
                NRF_LOG_ERROR("❌ Device ID write failed! Error Code: %d", err_code);
            }

        }
    } else if (p_evt_write->handle == m_ble_name_handles.value_handle) {
        uint16_t max_allowed_name_len = MAX_CUSTOM_NAME_LEN; // ✅ 8 characters max

        if (p_evt_write->len <= max_allowed_name_len) {
            memset(m_ble_name, 0, BLE_NAME_MAX_ADVERTISED);
            memcpy(m_ble_name, p_evt_write->data, p_evt_write->len);
            m_ble_name[p_evt_write->len] = '\0';

            NRF_LOG_INFO("Deleting old BLE Name before writing new one...");

            // ✅ Delete existing record if found
            if (fds_record_find(0xBEEF, 0x0002, &desc, &ftok) == NRF_SUCCESS) {
                fds_record_delete(&desc);
                fds_gc();  // ✅ Force garbage collection before writing
                NRF_LOG_INFO("Old BLE Name deleted.");
            }

            // ✅ Delay to ensure garbage collection completes
            nrf_delay_ms(100);

            NRF_LOG_INFO("Saving new BLE Name to Flash: %s", m_ble_name);

            fds_record_t record;
            record.file_id = 0xBEEF;
            record.key = 0x0002;
            record.data.p_data = m_ble_name;
            record.data.length_words = (p_evt_write->len + 4) / 4;

            err_code = fds_record_write(NULL, &record);
            if (err_code == NRF_SUCCESS) {
                NRF_LOG_INFO("✅ BLE Name successfully written to Flash.");
                // ✅ Reboot the device to apply the new BLE name
                NRF_LOG_INFO("Rebooting device to apply new BLE name...");
                nrf_delay_ms(500);  // ✅ Short delay before reset
                NVIC_SystemReset();  // ✅ Trigger a system reset
            } else {
                NRF_LOG_ERROR("❌ BLE Name write failed! Error Code: %d", err_code);
            }


        } else {
            NRF_LOG_WARNING("Rejected BLE Name: Too Long (Max %d chars)", MAX_CUSTOM_NAME_LEN);
        }
    }
}


/**@brief Function for initializing the custom BLE service. */
void ble_custom_service_init(void) {
    ble_uuid_t ble_uuid = {.uuid = CUSTOM_SERVICE_UUID, .type = BLE_UUID_TYPE_BLE};
    sd_ble_gatts_service_add(BLE_GATTS_SRVC_TYPE_PRIMARY, &ble_uuid, &m_service_handle);
    
    ble_add_char_params_t add_char_params = {0};

    // ✅ ANT+ Device ID - WRITE ONLY
    add_char_params.uuid = CUSTOM_CHAR_ANT_DEVICE_ID_UUID;
    add_char_params.uuid_type = BLE_UUID_TYPE_BLE;
    add_char_params.init_len = sizeof(m_ant_device_id);
    add_char_params.max_len = sizeof(m_ant_device_id);
    add_char_params.p_init_value = (uint8_t*)&m_ant_device_id;
    add_char_params.char_props.read = 0;  // ❌ Remove Read Permission
    add_char_params.char_props.write = 1; // ✅ Keep Write Permission
    add_char_params.read_access = SEC_NO_ACCESS;  // ❌ No Read Access
    add_char_params.write_access = SEC_OPEN;      // ✅ Allow Write
    characteristic_add(m_service_handle, &add_char_params, &m_ant_device_id_handles);

    // ✅ BLE Name - WRITE ONLY
    add_char_params.uuid = CUSTOM_CHAR_BLE_NAME_UUID;
    add_char_params.init_len = strlen(m_ble_name) + 1;
    add_char_params.max_len = BLE_NAME_MAX_ADVERTISED;
    add_char_params.p_init_value = (uint8_t*)m_ble_name;
    add_char_params.char_props.read = 0;  // ❌ Remove Read Permission
    add_char_params.char_props.write = 1; // ✅ Keep Write Permission
    add_char_params.read_access = SEC_NO_ACCESS;  // ❌ No Read Access
    add_char_params.write_access = SEC_OPEN;      // ✅ Allow Write
    characteristic_add(m_service_handle, &add_char_params, &m_ble_name_handles);
}


/**@brief Function to load stored values from flash memory. */
void custom_service_load_from_flash(void) {
    ret_code_t err_code;
    
    // Initialize FDS
    NRF_LOG_INFO("Initializing Flash Data Storage...");
    err_code = fds_init();
    APP_ERROR_CHECK(err_code);

    fds_record_desc_t desc;
    fds_find_token_t ftok = {0};
    fds_flash_record_t flash_record;

    // ✅ Try to find stored ANT Device ID
    NRF_LOG_INFO("Checking for stored ANT Device ID...");
    if (fds_record_find(0xBEEF, 0x0001, &desc, &ftok) == NRF_SUCCESS) {
        err_code = fds_record_open(&desc, &flash_record);
        if (err_code == NRF_SUCCESS && flash_record.p_data != NULL) {
            memcpy(&m_ant_device_id, flash_record.p_data, sizeof(m_ant_device_id));
            fds_record_close(&desc);

            NRF_LOG_INFO("Loaded Device ID from Flash: %d", m_ant_device_id);
        } else {
            NRF_LOG_WARNING("Failed to open FDS record for Device ID.");
        }
    } else {
        m_ant_device_id = DEFAULT_ANT_DEVICE_ID;
        NRF_LOG_WARNING("No stored Device ID found. Using default: %d", m_ant_device_id);
    }

    memset(&ftok, 0, sizeof(ftok)); // Reset token for next search

    // ✅ Try to find stored BLE Name
    NRF_LOG_INFO("Checking for stored BLE Name...");
    if (fds_record_find(0xBEEF, 0x0002, &desc, &ftok) == NRF_SUCCESS) {
        err_code = fds_record_open(&desc, &flash_record);
        if (err_code == NRF_SUCCESS && flash_record.p_data != NULL) {
            memset(m_ble_name, 0, BLE_NAME_MAX_ADVERTISED);

            // ✅ Find actual length of stored string (avoid reading padding)
            uint16_t stored_length = strlen((char*)flash_record.p_data);

            memcpy(m_ble_name, flash_record.p_data, stored_length);
            m_ble_name[stored_length] = '\0';

            fds_record_close(&desc);

            NRF_LOG_INFO("Loaded BLE Name from Flash: %s", m_ble_name);
        } else {
            NRF_LOG_WARNING("Failed to open FDS record for BLE Name.");
        }
    } else {
        memset(m_ble_name, 0, BLE_NAME_MAX_ADVERTISED);
        strncpy(m_ble_name, DEFAULT_BLE_NAME, BLE_NAME_MAX_ADVERTISED - 1);
        m_ble_name[BLE_NAME_MAX_ADVERTISED - 1] = '\0';

        NRF_LOG_WARNING("No stored BLE Name found. Using default: %s", m_ble_name);
    }

    // Apply BLE Name
    NRF_LOG_INFO("Construct new BLE Name: %s", m_ble_name);
    update_ble_name();
}



void ble_custom_service_on_ble_evt(ble_evt_t const * p_ble_evt, void * p_context)
{
    switch (p_ble_evt->header.evt_id)
    {
        case BLE_GAP_EVT_CONNECTED:
            m_conn_handle = p_ble_evt->evt.gap_evt.conn_handle;
            break;

        case BLE_GAP_EVT_DISCONNECTED:
            m_conn_handle = BLE_CONN_HANDLE_INVALID;
            break;

        case BLE_GATTS_EVT_WRITE:
            on_write(p_ble_evt);
            break;
        default:
            break;
    }
}

