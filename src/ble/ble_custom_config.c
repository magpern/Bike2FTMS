#include "ble.h"
#include "ble_srv_common.h"
#include "nrf_log.h"
#include "app_error.h"
#include "ble_custom_config.h"
#include "nrf_delay.h"
#include "nrf_pwr_mgmt.h"
#include "nrf_nvmc.h"
#include <string.h>
#include <nrf_sdh_ble.h>
#include "nrf_sdh.h"           // ✅ SoftDevice handling (nrf_sdh_enable_request, nrf_sdh_is_enabled)
#include "nrf_sdh_soc.h"       // ✅ System-on-Chip SoftDevice API (sd_softdevice_disable)
#include "fds.h"
#include "nrf_fstorage.h"      // ✅ Added for NRF_SUCCESS definition

#define CUSTOM_SERVICE_UUID          0x1523
#define CUSTOM_CHAR_DEVICE_INFO_UUID 0x1524  


#define UICR_DEVICE_ID_ADDR  ((uint32_t *)(NRF_UICR_BASE + offsetof(NRF_UICR_Type, CUSTOMER[0])))
#define UICR_BLE_NAME_ADDR   ((uint32_t *)(NRF_UICR_BASE + offsetof(NRF_UICR_Type, CUSTOMER[1])))

NRF_SDH_BLE_OBSERVER(m_custom_service_observer, APP_BLE_OBSERVER_PRIO, ble_custom_service_on_ble_evt, NULL);

static uint16_t m_service_handle;
static ble_gatts_char_handles_t m_device_info_handles;

// ✅ Global variables for access throughout the program
uint16_t m_ant_device_id = DEFAULT_ANT_DEVICE_ID;
char m_ble_name[BLE_NAME_MAX_LEN + 1] = DEFAULT_BLE_NAME;  
char ble_full_name[MAX_BLE_FULL_NAME_LEN] = {0};

#define CONFIG_FILE     (0x8010)
#define CONFIG_REC_KEY  (0x7010)

typedef struct {
    uint16_t device_id;
    char name[BLE_NAME_MAX_LEN + 1];
} device_config_t;

static bool fds_ready = false;
static bool fds_write_pending = false;

void save_device_config(void) {
    if (!fds_ready) {
        NRF_LOG_ERROR("FDS not ready");
        return;
    }

    static uint8_t data[12] __attribute__((aligned(4)));  // Make static and ensure alignment
    memset(data, 0, sizeof(data)); // Zero out buffer first

    // Store Device ID (Little-Endian)
    data[0] = (uint8_t)(m_ant_device_id & 0xFF);
    data[1] = (uint8_t)((m_ant_device_id >> 8) & 0xFF);

    // Copy BLE name and ensure padding with 0x00
    strncpy((char *)&data[2], m_ble_name, 8); // Copy name

    // Print byte-by-byte for debugging
    NRF_LOG_INFO("🔍 Data to be stored:");
    for (int i = 0; i < sizeof(data); i++) {
        NRF_LOG_INFO("Byte %d: 0x%02X", i, data[i]);
    }

    static fds_record_t record = {
        .file_id = CONFIG_FILE,
        .key = CONFIG_REC_KEY,
        .data = {
            .p_data = NULL,
            .length_words = (sizeof(data) + 3) / 4, // Convert to words
        }
    };
    record.data.p_data = data;  // Set the data pointer

    fds_record_desc_t desc = {0};
    fds_find_token_t ftok = {0};

    // Check if record exists and update
    ret_code_t ret = fds_record_find(CONFIG_FILE, CONFIG_REC_KEY, &desc, &ftok);
    if (ret == NRF_SUCCESS) {
        ret = fds_record_update(&desc, &record);
        if (ret != NRF_SUCCESS) {
            NRF_LOG_ERROR("🚨 Update failed, error: %d", ret);
            return;
        }
        fds_write_pending = true;  // Set pending flag for reboot
    } else {
        ret = fds_record_write(&desc, &record);
        if (ret != NRF_SUCCESS) {
            NRF_LOG_ERROR("🚨 Write failed, error: %d", ret);
            return;
        }
        fds_write_pending = true;  // Set pending flag for reboot
    }
}



void load_device_config(void) {
    if (!fds_ready) {
        NRF_LOG_WARNING("⚠️ FDS not ready, using defaults");
        m_ant_device_id = DEFAULT_ANT_DEVICE_ID;
        strncpy(m_ble_name, DEFAULT_BLE_NAME, 8);
        m_ble_name[8] = '\0';
        update_ble_name();
        return;
    }

    fds_record_desc_t desc = {0};
    fds_find_token_t  ftok = {0};
    ret_code_t ret = fds_record_find(CONFIG_FILE, CONFIG_REC_KEY, &desc, &ftok);

    if (ret == NRF_SUCCESS) {
        NRF_LOG_INFO("📄 Found stored config at address: 0x%08X", desc.p_record);
        fds_flash_record_t record;
        ret = fds_record_open(&desc, &record);

        if (ret == NRF_SUCCESS) {
            uint8_t *data = (uint8_t *)record.p_data;

            // **Verify byte order before parsing**
            m_ant_device_id = (uint16_t)(data[0] | (data[1] << 8));
            NRF_LOG_INFO("✅ Parsed Device ID: %d (0x%04X)", m_ant_device_id, m_ant_device_id);

            // **Check if name is missing**
            memcpy(m_ble_name, &data[2], 8);
            m_ble_name[8] = '\0';
            NRF_LOG_INFO("✅ Parsed BLE Name: %s", m_ble_name);

            fds_record_close(&desc);
        } else {
            NRF_LOG_ERROR("🚨 Failed to open record!");
        }
    } else {
        NRF_LOG_WARNING("⚠️ No stored config found, using defaults");
        m_ant_device_id = DEFAULT_ANT_DEVICE_ID;
        strncpy(m_ble_name, DEFAULT_BLE_NAME, 8);
        m_ble_name[8] = '\0';
    }

    update_ble_name();
}



static void fds_evt_handler(fds_evt_t const * p_evt) {
    switch (p_evt->id) {
        case FDS_EVT_INIT:
            if (p_evt->result == NRF_SUCCESS) {
                fds_ready = true;
                NRF_LOG_INFO("✅ FDS initialized and ready");
                load_device_config();  // **Only load config after FDS is ready**
            } else {
                NRF_LOG_ERROR("🚨 FDS initialization failed: %d", p_evt->result);
            }
            break;

        case FDS_EVT_WRITE:
            if (p_evt->result == NRF_SUCCESS) {
                NRF_LOG_INFO("✅ Write successful, verifying data...");
                
                NRF_LOG_INFO("Rebooting to apply changes...");
                nrf_delay_ms(100);
                NVIC_SystemReset();
            } else {
                NRF_LOG_ERROR("🚨 FDS write failed! Error: %d", p_evt->result);
            }
            break;

        case FDS_EVT_UPDATE:
            if (p_evt->result == NRF_SUCCESS) {
                NRF_LOG_INFO("✅ Config updated successfully");
                nrf_delay_ms(100);
                NVIC_SystemReset();
            } else {
                NRF_LOG_ERROR("🚨 FDS update failed! Error: %d", p_evt->result);
            }
            break;

        case FDS_EVT_DEL_RECORD:
            if (p_evt->result == NRF_SUCCESS) {
                NRF_LOG_INFO("🗑️ Record deleted successfully, starting garbage collection...");
                fds_gc();  // **Run GC after deletion**
            } else {
                NRF_LOG_ERROR("🚨 FDS delete failed! Error: %d", p_evt->result);
            }
            break;

        case FDS_EVT_GC:
            if (p_evt->result == NRF_SUCCESS) {
                NRF_LOG_INFO("✅ Garbage collection completed, now writing new record...");
                save_device_config();  // **Trigger save after GC completes**
            } else {
                NRF_LOG_ERROR("🚨 Garbage collection failed! Error: %d", p_evt->result);
            }
            break;

        default:
            NRF_LOG_INFO("ℹ️ Unhandled FDS Event: %d, Result: %d", p_evt->id, p_evt->result);
            break;
    }
}



void update_ble_name() {
    snprintf(ble_full_name, sizeof(ble_full_name), "%.*s_%05d",
             BLE_NAME_MAX_LEN, m_ble_name, m_ant_device_id);
    
    NRF_LOG_INFO("Updated BLE Full Name: %s", ble_full_name);
}

/**@brief   Wait for fds to initialize. */
static void wait_for_fds_ready(void)
{
    while (!fds_ready)
    {
        sd_app_evt_wait();
    }
}

void custom_service_init(void) {
    ret_code_t ret = fds_register(fds_evt_handler);
    APP_ERROR_CHECK(ret);
    
    ret = fds_init();
    APP_ERROR_CHECK(ret);

    wait_for_fds_ready();
}



/**@brief Function for handling BLE writes. */
static void on_write(ble_evt_t const *p_ble_evt) {
    ble_gatts_evt_write_t const *p_evt_write = &p_ble_evt->evt.gatts_evt.params.write;

    if (p_evt_write->handle == m_device_info_handles.value_handle) {
        uint16_t write_len = p_evt_write->len;

        if (write_len < 3 || write_len > BLE_NAME_MAX_LEN + 3) {
            NRF_LOG_WARNING("Invalid Data Length: %d bytes", write_len);
            return;
        }

        // Update global Device ID
        m_ant_device_id = (uint16_t)(p_evt_write->data[0] | (p_evt_write->data[1] << 8));

        // Extract BLE Name
        uint8_t name_length = p_evt_write->data[2];
        if (name_length > BLE_NAME_MAX_LEN) name_length = BLE_NAME_MAX_LEN;

        memset(m_ble_name, 0, BLE_NAME_MAX_LEN + 1);
        memcpy(m_ble_name, &p_evt_write->data[3], name_length);
        m_ble_name[name_length] = '\0';

        NRF_LOG_INFO("New Device ID: %d", m_ant_device_id);
        NRF_LOG_INFO("New BLE Name: %s", m_ble_name);

        // Save to FDS - will reboot after successful write
        save_device_config();
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
    add_char_params.init_len = BLE_NAME_MAX_LEN + 3;
    add_char_params.max_len = BLE_NAME_MAX_LEN + 3;
    add_char_params.char_props.read = 1;
    add_char_params.char_props.write = 1;
    add_char_params.read_access = SEC_OPEN;
    add_char_params.write_access = SEC_OPEN;
    characteristic_add(m_service_handle, &add_char_params, &m_device_info_handles);

    // ✅ Set initial value after adding the characteristic
    uint8_t initial_value[BLE_NAME_MAX_LEN + 3] = {0};

    initial_value[0] = (uint8_t)(m_ant_device_id & 0xFF);
    initial_value[1] = (uint8_t)((m_ant_device_id >> 8) & 0xFF);
    initial_value[2] = strlen(m_ble_name);
    memcpy(&initial_value[3], m_ble_name, initial_value[2]);

    ble_gatts_value_t value = {
        .len = sizeof(initial_value),
        .offset = 0,
        .p_value = initial_value
    };

    sd_ble_gatts_value_set(BLE_CONN_HANDLE_INVALID, m_device_info_handles.value_handle, &value);
}


/**@brief Function to initialize and load stored values */
void custom_service_load_from_flash(void) {
    NRF_LOG_INFO("Loading stored Device Config from FDS...");
    load_device_config();
}
