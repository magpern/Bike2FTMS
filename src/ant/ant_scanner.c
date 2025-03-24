#include "ant_scanner.h"

#include "ant_interface.h"
#include "ant_parameters.h"
#include "ant_channel_config.h"
#include "ant_error.h"
#include "nrf_sdh_ant.h"
#include "nrf_log.h"
#include "common_definitions.h"

#define ANTPLUS_NETWORK_NUMBER     0  // Use network 0 by convention
#define SCAN_CHANNEL_NUMBER        0
#define SCAN_DEVICE_NUMBER         0  // 0 = wildcard
#define SCAN_DEVICE_TYPE           0  // 0 = wildcard
#define SCAN_TRANS_TYPE            0  // 0 = wildcard
#define SCAN_CHANNEL_PERIOD        8192   // e.g. ~4 Hz
#define SCAN_RF_FREQUENCY          57     // 2457 MHz (standard ANT+)

static void scan_ant_evt_handler(ant_evt_t * p_ant_evt, void * p_context);

/**@brief Observer to catch all ANT events for this scanning channel. */
NRF_SDH_ANT_OBSERVER(m_ant_scanner_observer,
                     APP_ANT_OBSERVER_PRIO,
                     scan_ant_evt_handler,
                     NULL);

/**@brief Function to configure a wildcard scanning channel and open it. */
void ant_scanner_init(void)
{
    uint32_t err_code;

    // 1) Set the ANT+ Network Key (for network 0).
    err_code = sd_ant_network_address_set(ANTPLUS_NETWORK_NUMBER, ANT_PLUS_NETWORK_KEY);
    APP_ERROR_CHECK(err_code);

    // 2) First assign channel 0 - this is required before scan mode
    err_code = sd_ant_channel_assign(SCAN_CHANNEL_NUMBER, 
                                    CHANNEL_TYPE_SLAVE, 
                                    ANTPLUS_NETWORK_NUMBER,
                                    1); // Extended assignments
    APP_ERROR_CHECK(err_code);

    // 3) Configure the RF frequency
    err_code = sd_ant_channel_radio_freq_set(SCAN_CHANNEL_NUMBER, SCAN_RF_FREQUENCY);
    APP_ERROR_CHECK(err_code);
    
    // 4) Start scan mode with parameter 0 (allowing all packets, not just synchronous ones)
    err_code = sd_ant_rx_scan_mode_start(0);
    APP_ERROR_CHECK(err_code);

    NRF_LOG_INFO("ANT Scanner: channel %u opened for wildcard scanning.", SCAN_CHANNEL_NUMBER);
}

/**@brief Handler for any ANT events on our scan channel. */
static void scan_ant_evt_handler(ant_evt_t * p_ant_evt, void * p_context)
{
    // Print something at the start so we know the handler was called.
    NRF_LOG_INFO("scan_ant_evt_handler called.");

    if (p_ant_evt->channel != SCAN_CHANNEL_NUMBER)
    {
        // If you have multiple channels, ignore events for others
        char info_str[64];
        snprintf(info_str, sizeof(info_str),
                 "Got ANT evt on ch=%u (wanted=%u), evt=%u",
                 p_ant_evt->channel,
                 SCAN_CHANNEL_NUMBER,
                 p_ant_evt->event);
        NRF_LOG_INFO("%s", info_str);
        return;
    }

    switch (p_ant_evt->event)
    {
        case EVENT_RX:
        {
            // Standard 8-byte payload location:
            const uint8_t * p_payload = p_ant_evt->message
                                              .stMessage
                                              .uFramedData
                                              .stFramedData
                                              .uMesgData
                                              .stMesgData
                                              .aucPayload;

            // Attempt to read the channel ID for dev#, dev_type, trans_type
            uint16_t dev_number = 0xFFFF;
            uint8_t  dev_type   = 0xFF;
            uint8_t  trans_type = 0xFF;

            uint32_t err_code = sd_ant_channel_id_get(SCAN_CHANNEL_NUMBER,
                                                      &dev_number,
                                                      &dev_type,
                                                      &trans_type);

            if (err_code == NRF_SUCCESS)
            {
                // Format the info into a single string so we only have 1 placeholder in the log
                char info_str[128];
                snprintf(info_str, sizeof(info_str),
                         "ANT RX (Ch=%u) dev=%u type=%u trans=%u payload=[%02X %02X %02X %02X %02X %02X %02X %02X]",
                         p_ant_evt->channel,
                         dev_number, dev_type, trans_type,
                         p_payload[0], p_payload[1], p_payload[2], p_payload[3],
                         p_payload[4], p_payload[5], p_payload[6], p_payload[7]);
                NRF_LOG_INFO("%s", info_str);
            }
            else
            {
                // If we can’t get the channel ID, still show payload
                char info_str[128];
                snprintf(info_str, sizeof(info_str),
                         "ANT RX IDgetErr=%lu payload=[%02X %02X %02X %02X %02X %02X %02X %02X]",
                         err_code,
                         p_payload[0], p_payload[1], p_payload[2], p_payload[3],
                         p_payload[4], p_payload[5], p_payload[6], p_payload[7]);
                NRF_LOG_WARNING("%s", info_str);
            }
        }
        break;

        case EVENT_RX_SEARCH_TIMEOUT:
        {
            NRF_LOG_INFO("ANT Scanner: search timed out.");
        }
        break;

        case EVENT_CHANNEL_CLOSED:
        {
            NRF_LOG_INFO("ANT Scanner: channel closed.");
        }
        break;

        default:
        {
            char info_str[64];
            snprintf(info_str, sizeof(info_str),
                     "ANT Scanner: unhandled event %u", p_ant_evt->event);
            NRF_LOG_INFO("%s", info_str);
        }
        break;
    }
}

