#include "ble.h"
#include "ble_srv_common.h"
#include "nrf_log.h"
#include "app_error.h"
#include "ble_custom_config.h"
#include "nrf_delay.h"
#include "nrf_pwr_mgmt.h"
#include "nrf_nvmc.h"
#include <string.h>

#define CUSTOM_SERVICE_UUID          0x1523
#define CUSTOM_CHAR_DEVICE_INFO_UUID 0x1524  


#define UICR_DEVICE_ID_ADDR  ((uint32_t *)(NRF_UICR_BASE + offsetof(NRF_UICR_Type, CUSTOMER[0])))
#define UICR_BLE_NAME_ADDR   ((uint32_t *)(NRF_UICR_BASE + offsetof(NRF_UICR_Type, CUSTOMER[1])))

static uint16_t m_service_handle;
static ble_gatts_char_handles_t m_device_info_handles;

// ✅ Global variables for access throughout the program
uint16_t m_ant_device_id = DEFAULT_ANT_DEVICE_ID;
char m_ble_name[BLE_NAME_MAX_LEN + 1] = DEFAULT_BLE_NAME;  
char ble_full_name[MAX_BLE_FULL_NAME_LEN] = {0};

void update_ble_name() {
    snprintf(ble_full_name, sizeof(ble_full_name), "%.*s_%05d",
             BLE_NAME_MAX_LEN, m_ble_name, m_ant_device_id);
    
    NRF_LOG_INFO("Updated BLE Full Name: %s", ble_full_name);
}

/**@brief Save config values to UICR */
void save_device_config() {
    uint32_t uicr_data[3];  // ✅ Store data in 32-bit words
    uicr_data[0] = m_ant_device_id;
    memcpy(&uicr_data[1], m_ble_name, BLE_NAME_MAX_LEN);

    // ✅ Erase UICR page first (mandatory)
    if (NRF_UICR->CUSTOMER[0] != 0xFFFFFFFF || NRF_UICR->CUSTOMER[1] != 0xFFFFFFFF) {
        NRF_LOG_WARNING("UICR not empty, erasing...");
        nrf_nvmc_page_erase(NRF_UICR_BASE);
    }

    // ✅ Write new data
    nrf_nvmc_write_words((uint32_t)UICR_DEVICE_ID_ADDR, uicr_data, 3);

    NRF_LOG_INFO("✅ Device ID and BLE Name stored in UICR.");
}

/**@brief Load stored values from UICR */
void load_device_config() {
    if (*(uint32_t *)UICR_DEVICE_ID_ADDR != 0xFFFFFFFF) {  // Check if UICR is empty
        m_ant_device_id = (uint16_t)(*(uint32_t *)UICR_DEVICE_ID_ADDR);
        strncpy(m_ble_name, (char *)UICR_BLE_NAME_ADDR, BLE_NAME_MAX_LEN);
        m_ble_name[BLE_NAME_MAX_LEN] = '\0';
        NRF_LOG_INFO("Loaded Device ID: %d", m_ant_device_id);
        NRF_LOG_INFO("Loaded BLE Name: %s", m_ble_name);
    } else {
        m_ant_device_id = DEFAULT_ANT_DEVICE_ID;
        strncpy(m_ble_name, DEFAULT_BLE_NAME, BLE_NAME_MAX_LEN);
        m_ble_name[BLE_NAME_MAX_LEN] = '\0';
        NRF_LOG_WARNING("No stored values found, using defaults.");
    }
    update_ble_name();
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

        // ✅ Update global Device ID
        m_ant_device_id = (uint16_t)(p_evt_write->data[0] | (p_evt_write->data[1] << 8));

        // ✅ Extract BLE Name
        uint8_t name_length = p_evt_write->data[2];
        if (name_length > BLE_NAME_MAX_LEN) name_length = BLE_NAME_MAX_LEN;

        memset(m_ble_name, 0, BLE_NAME_MAX_LEN + 1);
        memcpy(m_ble_name, &p_evt_write->data[3], name_length);
        m_ble_name[name_length] = '\0';

        NRF_LOG_INFO("New Device ID: %d", m_ant_device_id);
        NRF_LOG_INFO("New BLE Name: %s", m_ble_name);

        // ✅ Save to UICR
        save_device_config();

        // ✅ Reboot to apply changes
        NRF_LOG_INFO("Rebooting device to apply new BLE name...");
        nrf_delay_ms(500);
        NVIC_SystemReset();
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
    add_char_params.p_init_value = NULL;
    add_char_params.char_props.read = 1;
    add_char_params.char_props.write = 1;
    add_char_params.read_access = SEC_OPEN;
    add_char_params.write_access = SEC_OPEN;
    characteristic_add(m_service_handle, &add_char_params, &m_device_info_handles);
}

/**@brief Function to initialize and load stored values */
void custom_service_load_from_flash(void) {
    NRF_LOG_INFO("Loading stored Device Config from UICR...");
    load_device_config();
}
