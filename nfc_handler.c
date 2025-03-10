#include "nfc_handler.h"
#include "nfc_t2t_lib.h"
#include "nrf_log.h"
#include "ble_custom_config.h"
#include "nrf_delay.h"
#include "nrf_nvmc.h"
#include <ctype.h>  // Required for hex conversion
#include <stdlib.h>  // ✅ Required for strtol()
#include "nrfx_nfct.h"  // ✅ Required for nrfx_nfct functions

#define NFC_DRY_RUN_MODE 1  // ✅ Set to 1 to enable dry-run, 0 to disable

static uint8_t nfc_rx_buffer[32];  // Buffer for NFC data
static uint8_t nfc_data_length;    // Length of received data

/**@brief Convert hex string to byte array */
static bool hex_string_to_bytes(const uint8_t *hex_str, size_t hex_len, uint8_t *out_bytes, size_t *out_len)
{
    if (hex_len % 2 != 0 || hex_len / 2 > *out_len) {
        return false;  // Invalid length or buffer too small
    }

    for (size_t i = 0; i < hex_len / 2; i++) {
        char hex_byte[3] = { hex_str[i * 2], hex_str[i * 2 + 1], '\0' };
        out_bytes[i] = (uint8_t)strtol(hex_byte, NULL, 16);
    }

    *out_len = hex_len / 2;
    return true;
}

static void nrfx_nfc_event_handler(nrfx_nfct_evt_t const * p_evt)
{
    if (p_evt->evt_id == NRFX_NFCT_EVT_FIELD_DETECTED)
    {
        NRF_LOG_INFO("📡 NFC Field Detected (Reader Mode).");
    }
    else if (p_evt->evt_id == NRFX_NFCT_EVT_FIELD_LOST)
    {
        NRF_LOG_INFO("📡 NFC Field Lost.");
    }
    else if (p_evt->evt_id == NRFX_NFCT_EVT_RX_FRAMEEND)
    {
        NRF_LOG_INFO("📡 NFC Data Received!");

        // Extract received data
        uint8_t const *rx_data = p_evt->params.rx_frameend.rx_data.p_data;
        size_t rx_data_len = p_evt->params.rx_frameend.rx_data.data_size;

        if (rx_data_len > 0 && rx_data_len <= sizeof(nfc_rx_buffer))
        {
            memcpy(nfc_rx_buffer, rx_data, rx_data_len);
            nfc_data_length = rx_data_len;

            NRF_LOG_INFO("📡 NFC Data Length: %d", rx_data_len);
            NRF_LOG_HEXDUMP_INFO(nfc_rx_buffer, rx_data_len);

            // ✅ Parsing Logic (Moved from nfc_callback)
            uint8_t binary_data[32] = {0};
            size_t binary_length = sizeof(binary_data);

            // Convert hex-encoded text to binary
            if (!hex_string_to_bytes(nfc_rx_buffer, nfc_data_length, binary_data, &binary_length)) {
                NRF_LOG_WARNING("🚨 Failed to parse NFC data as hex.");
                return;
            }

            if (binary_length >= 3)
            {
                uint16_t device_id = (binary_data[0] << 8) | binary_data[1];  // Extract device ID
                uint8_t name_len = binary_data[2];  // Extract length of device name

                if (name_len > binary_length - 3)
                {
                    NRF_LOG_WARNING("🚨 Invalid name length!");
                    return;
                }

                char device_name[16] = {0};
                memcpy(device_name, &binary_data[3], name_len);
                device_name[name_len] = '\0';

                NRF_LOG_INFO("📡 NFC Parsed Data: Device ID: %d, Name: %s", device_id, device_name);

#if NFC_DRY_RUN_MODE == 0
                // ✅ Actual Update (Only if Dry-Run Mode is OFF)
                m_ant_device_id = device_id;
                strncpy(m_ble_name, device_name, name_len);
                save_device_config();
                update_ble_name();

                // Restart BLE to apply changes
                NRF_LOG_INFO("🔄 Rebooting to apply NFC config...");
                nrf_delay_ms(500);
                NVIC_SystemReset();
#endif
            }
        }
        else
        {
            NRF_LOG_WARNING("🚨 NFC RX Data Length Out of Range: %d", rx_data_len);
        }
    }
    else if (p_evt->evt_id == NRFX_NFCT_EVT_ERROR)
    {
        NRF_LOG_ERROR("🚨 NFC Error! Code: 0x%08X", p_evt->params.error.reason);
    } else {
        NRF_LOG_INFO("🔍 Unknown NFC Event ID: 0x%08X", p_evt->evt_id);
    }	
}




void nfc_init(void)
{
    NRF_LOG_INFO("🏁 Starting NFC Reader...");

    // ✅ Initialize NFC in Reader Mode with RXFRAMEEND interrupt enabled
    nrfx_nfct_config_t nfct_config = {
        .rxtx_int_mask = NRF_NFCT_INT_RXFRAMEEND_MASK,  // ✅ Correct mask for RX data events
        .cb = nrfx_nfc_event_handler   // Use the correct callback for RX data
    };

    nrfx_nfct_disable();  // Reset NFC hardware before setup

    nrfx_err_t err_code = nrfx_nfct_init(&nfct_config);
    if (err_code != NRFX_SUCCESS)
    {
        NRF_LOG_ERROR("🚨 NFC Init Failed! Error Code: 0x%08X", err_code);
        return;
    }

    NRF_LOG_INFO("✅ NFC Initialized. Enabling Reader Mode...");
    nrfx_nfct_enable();
    NRF_NFCT->PACKETPTR = (uint32_t)nfc_rx_buffer;
    NRF_NFCT->TASKS_ACTIVATE = 1;
}









