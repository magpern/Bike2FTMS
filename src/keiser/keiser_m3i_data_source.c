#include "keiser_m3i_data_source.h"
#include "nrf_log.h"
#include "app_timer.h"
#include "cycling_data_model.h"
#include "nrf_sdh_ble.h"
#include "nrf_sdh.h"
#include "ble.h"
#include "ble_custom_config.h"

// Define the BLE observer priority for our module
#define KEISER_M3I_BLE_OBSERVER_PRIO 2

// Forward declarations
static void timeout_timer_handler(void *p_context);
static void process_adv_data(const ble_gap_evt_adv_report_t *p_adv_report);
static void ble_evt_handler(ble_evt_t const *p_ble_evt, void *p_context);

// Register our BLE event handler with the SoftDevice
NRF_SDH_BLE_OBSERVER(m_keiser_m3i_ble_observer, KEISER_M3I_BLE_OBSERVER_PRIO, ble_evt_handler, NULL);

// Static variables
static data_source_config_t m_config;
static keiser_m3i_config_t m_keiser_config = {
    .target_mac = {0x24, 0xEC, 0x4A, 0x2C, 0x6E, 0xE1}  // Initialize with MAC from ble_custom_config.h
};
static bool m_is_active = false;
static app_timer_id_t m_timeout_timer_id;
static keiser_m3i_data_t m_last_data = {0};
static uint8_t m_adv_report_buffer[BLE_GAP_SCAN_BUFFER_MIN];  // Buffer for advertising reports
static ble_gap_scan_params_t m_scan_params = {
    .active = 1,  // Active scanning
    .interval = MSEC_TO_UNITS(100, UNIT_0_625_MS),  // Scan interval
    .window = MSEC_TO_UNITS(100, UNIT_0_625_MS),    // Scan window
    .timeout = 0,  // No timeout
    .filter_policy = BLE_GAP_SCAN_FP_ACCEPT_ALL,  // Accept all advertisements
    .scan_phys = BLE_GAP_PHY_1MBPS  // Use 1M PHY
};
static ble_data_t m_adv_report = {
    .p_data = m_adv_report_buffer,
    .len = BLE_GAP_SCAN_BUFFER_MIN
};

// Initialize the timeout timer
static void init_timeout_timer(void)
{
    uint32_t err_code = app_timer_create(&m_timeout_timer_id,
                                       APP_TIMER_MODE_SINGLE_SHOT,
                                       timeout_timer_handler);
    APP_ERROR_CHECK(err_code);
}

// Timeout handler - called when we haven't received data for too long
static void timeout_timer_handler(void *p_context)
{
    m_is_active = false;
    NRF_LOG_INFO("Keiser M3i data source timeout - no data received");
    
    // Notify data manager of timeout
    if (m_config.data_callback != NULL)
    {
        m_config.data_callback(0, 0);  // Send zero values to indicate timeout
    }
}

// Check if MAC addresses match
static bool mac_address_match(const uint8_t *mac1, const uint8_t *mac2)
{
    // Compare bytes in reverse order (BLE MAC is little-endian)
    for (uint8_t i = 0; i < BLE_GAP_ADDR_LEN; i++)
    {
        if (mac1[i] != mac2[BLE_GAP_ADDR_LEN - 1 - i])
        {
            return false;
        }
    }

    return true;
}


// Parse Keiser M3i data from manufacturer specific data
static bool parse_keiser_data(const uint8_t *p_data, uint8_t data_len, keiser_m3i_data_t *p_keiser_data)
{
    if (p_data == NULL || p_keiser_data == NULL || data_len < 17) {
        NRF_LOG_INFO("Keiser M3i: Invalid parameters for parsing");
        return false;
    }

    p_keiser_data->version_major = p_data[0];
    p_keiser_data->version_minor = p_data[1];
    p_keiser_data->data_type = p_data[2];
    p_keiser_data->equipment_id = p_data[3];
    p_keiser_data->cadence = (p_data[5] << 8) | p_data[4];
    p_keiser_data->heart_rate = (p_data[7] << 8) | p_data[6];
    p_keiser_data->power = (p_data[9] << 8) | p_data[8];
    p_keiser_data->calories = (p_data[11] << 8) | p_data[10];
    p_keiser_data->duration_min = p_data[12];
    p_keiser_data->duration_sec = p_data[13];
    p_keiser_data->distance = (p_data[15] << 8) | p_data[14];
    p_keiser_data->gear = p_data[16];

    return true;
}

// Process advertising data from Keiser M3i
static void process_adv_data(const ble_gap_evt_adv_report_t *p_adv_report)
{
    uint8_t *p_data = p_adv_report->data.p_data;
    uint8_t data_len = p_adv_report->data.len;

    for (uint8_t i = 0; i < data_len - 1; i++)
    {
        if (p_data[i] != 0xFF) continue;  // Skip if not manufacturer data

        if (i + 3 > data_len) continue;  // Not enough for manufacturer ID

        uint16_t manufacturer_id = (p_data[i + 2] << 8) | p_data[i + 1];
        if (manufacturer_id != KEISER_M3I_MANUFACTURER_ID) continue;

        // Now check MAC address
        if (!mac_address_match(p_adv_report->peer_addr.addr, m_keiser_config.target_mac))
            break;  // Not our target device

        // Format MAC string for logging
        char mac_str[18];
        snprintf(mac_str, sizeof(mac_str), "%02X:%02X:%02X:%02X:%02X:%02X",
                 p_adv_report->peer_addr.addr[5], p_adv_report->peer_addr.addr[4],
                 p_adv_report->peer_addr.addr[3], p_adv_report->peer_addr.addr[2],
                 p_adv_report->peer_addr.addr[1], p_adv_report->peer_addr.addr[0]);

        NRF_LOG_DEBUG("Keiser M3i: Found manufacturer data 0x%04X from %s",
                     manufacturer_id, mac_str);

        if (i + 20 > data_len)
        {
            NRF_LOG_WARNING("Keiser M3i: Not enough bytes for Keiser data from %s", mac_str);
            break;
        }

        keiser_m3i_data_t new_data;
        new_data.manufacturer_id = manufacturer_id;

        if (!parse_keiser_data(&p_data[i + 3], 17, &new_data)) break;

        NRF_LOG_DEBUG("Keiser M3i: Power: %d W, Cadence: %d RPM from %s",
                     new_data.power, new_data.cadence / 10, mac_str);

        m_last_data = new_data;
        m_is_active = true;

        if (m_config.data_callback)
        {
            uint8_t cadence_rpm = new_data.cadence / 10;
            m_config.data_callback(new_data.power, cadence_rpm);
        }
        else
        {
            NRF_LOG_WARNING("Keiser M3i: No data callback registered!");
        }

        uint32_t err_code = app_timer_stop(m_timeout_timer_id);
        APP_ERROR_CHECK(err_code);
        err_code = app_timer_start(m_timeout_timer_id,
                                   APP_TIMER_TICKS(KEISER_M3I_ADV_TIMEOUT_MS),
                                   NULL);
        APP_ERROR_CHECK(err_code);

        break;  // We processed our target device
    }

    // Resume scanning
    uint32_t err_code = sd_ble_gap_scan_start(NULL, &m_adv_report);
    if (err_code != NRF_SUCCESS)
    {
        NRF_LOG_ERROR("Keiser M3i: Failed to continue scanning: %d", err_code);
    }
}




// BLE event handler
static void ble_evt_handler(ble_evt_t const *p_ble_evt, void *p_context)
{
    switch (p_ble_evt->header.evt_id)
    {
        case BLE_GAP_EVT_ADV_REPORT:
            process_adv_data(&p_ble_evt->evt.gap_evt.params.adv_report);
            break;

        default:
            break;
    }
}

// Initialize the Keiser M3i data source
bool keiser_m3i_init(data_source_config_t* config)
{
    if (config == NULL) {
        NRF_LOG_ERROR("Keiser M3i: Invalid configuration");
        return false;
    }
    
    m_config = *config;
    m_is_active = false;
    
    // Initialize timeout timer
    init_timeout_timer();
    
    // The BLE event handler is now automatically registered through NRF_SDH_BLE_OBSERVER
    // No need to call softdevice_ble_evt_handler_set directly
    
    NRF_LOG_INFO("Keiser M3i: Initialized with MAC %02X:%02X:%02X:%02X:%02X:%02X",
                 m_keiser_config.target_mac[0], m_keiser_config.target_mac[1],
                 m_keiser_config.target_mac[2], m_keiser_config.target_mac[3],
                 m_keiser_config.target_mac[4], m_keiser_config.target_mac[5]);
    
    return true;
}

// Start the Keiser M3i data source
bool keiser_m3i_start(void)
{
    // First check if BLE stack is enabled
    if (!nrf_sdh_is_enabled())
    {
        NRF_LOG_ERROR("Keiser M3i: BLE stack not enabled");
        return false;
    }

    // Verify MAC address is set
    bool mac_is_zero = true;
    for (uint8_t i = 0; i < BLE_GAP_ADDR_LEN; i++)
    {
        if (m_keiser_config.target_mac[i] != 0)
        {
            mac_is_zero = false;
            break;
        }
    }
    
    if (mac_is_zero)
    {
        NRF_LOG_ERROR("Keiser M3i: MAC address not set");
        return false;
    }

    NRF_LOG_INFO("Keiser M3i: Starting scan for MAC %02X:%02X:%02X:%02X:%02X:%02X",
                 m_keiser_config.target_mac[5], m_keiser_config.target_mac[4],
                 m_keiser_config.target_mac[3], m_keiser_config.target_mac[2],
                 m_keiser_config.target_mac[1], m_keiser_config.target_mac[0]);

    // Stop any existing scan first
    uint32_t err_code = sd_ble_gap_scan_stop();
    if (err_code != NRF_SUCCESS && err_code != NRF_ERROR_INVALID_STATE)
    {
        NRF_LOG_ERROR("Keiser M3i: Failed to stop existing scan: %d", err_code);
        return false;
    }
    
    // Start scanning with our static parameters
    err_code = sd_ble_gap_scan_start(&m_scan_params, &m_adv_report);
    if (err_code != NRF_SUCCESS)
    {
        NRF_LOG_ERROR("Keiser M3i: Failed to start BLE scanning: %d", err_code);
        return false;
    }
    
    NRF_LOG_INFO("Keiser M3i: Started BLE scanning");
    m_is_active = true;
    return true;
}

// Stop the Keiser M3i data source
void keiser_m3i_stop(void)
{
    // Stop scanning
    uint32_t err_code = sd_ble_gap_scan_stop();
    APP_ERROR_CHECK(err_code);
    
    // Stop timeout timer
    err_code = app_timer_stop(m_timeout_timer_id);
    APP_ERROR_CHECK(err_code);
    
    m_is_active = false;
}

// Check if the Keiser M3i data source is active
bool keiser_m3i_is_active(void)
{
    return m_is_active;
}

// Get the interface for the Keiser M3i data source
const data_source_interface_t* keiser_m3i_data_source_get_interface(void)
{
    static const data_source_interface_t interface = {
        .init = keiser_m3i_init,
        .start = keiser_m3i_start,
        .stop = keiser_m3i_stop,
        .is_active = keiser_m3i_is_active
    };
    
    return &interface;
} 