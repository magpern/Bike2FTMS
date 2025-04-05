#include <stdint.h>
#include "boards.h"
#include "nrf_mbr.h"
#include "nrf_bootloader.h"
#include "nrf_bootloader_app_start.h"
#include "nrf_bootloader_dfu_timers.h"
#include "nrf_dfu.h"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
#include "app_error.h"
#include "app_error_weak.h"
#include "nrf_bootloader_info.h"
#include "nrf_delay.h"
#include "app_timer.h"

#define DFU_INIT_TIMEOUT_MS       (30 * 1000)
#define DFU_DISCONNECT_TIMEOUT_MS (10 * 1000)

APP_TIMER_DEF(m_dfu_timeout_timer);
static bool m_is_connected = false;

static void dfu_timeout_handler(void * p_context)
{
    NRF_LOG_WARNING("⏱️ DFU timeout — rebooting device.");
    NRF_LOG_FINAL_FLUSH();
    NVIC_SystemReset();
}

static void start_dfu_timeout(uint32_t timeout_ms)
{
    app_timer_stop(m_dfu_timeout_timer);
    app_timer_start(m_dfu_timeout_timer, APP_TIMER_TICKS(timeout_ms), NULL);
}

static void on_error(void)
{
    NRF_LOG_FINAL_FLUSH();
#if NRF_MODULE_ENABLED(NRF_LOG_BACKEND_RTT)
    nrf_delay_ms(100);
#endif
#ifdef NRF_DFU_DEBUG_VERSION
    NRF_BREAKPOINT_COND;
#endif
    NVIC_SystemReset();
}

void app_error_handler(uint32_t error_code, uint32_t line_num, const uint8_t * p_file_name)
{
    NRF_LOG_ERROR("%s:%d", p_file_name, line_num);
    on_error();
}

void app_error_fault_handler(uint32_t id, uint32_t pc, uint32_t info)
{
    NRF_LOG_ERROR("Fault! id: 0x%08x, pc: 0x%08x, info: 0x%08x", id, pc, info);
    on_error();
}

void app_error_handler_bare(uint32_t error_code)
{
    NRF_LOG_ERROR("Error: 0x%08x!", error_code);
    on_error();
}

static void dfu_observer(nrf_dfu_evt_type_t evt_type)
{
    switch (evt_type)
    {
        case NRF_DFU_EVT_DFU_FAILED:
        case NRF_DFU_EVT_DFU_ABORTED:
        case NRF_DFU_EVT_DFU_INITIALIZED:
            bsp_board_init(BSP_INIT_LEDS);
            bsp_board_led_on(BSP_BOARD_LED_0);
            bsp_board_led_on(BSP_BOARD_LED_1);
            bsp_board_led_off(BSP_BOARD_LED_2);

            // Start failsafe timer if no connection
            if (!m_is_connected)
            {
                start_dfu_timeout(DFU_INIT_TIMEOUT_MS);
            }
            break;

        case NRF_DFU_EVT_TRANSPORT_ACTIVATED:
            bsp_board_led_off(BSP_BOARD_LED_1);
            bsp_board_led_on(BSP_BOARD_LED_2);
            m_is_connected = true;

            app_timer_stop(m_dfu_timeout_timer);
            break;

        case NRF_DFU_EVT_TRANSPORT_DEACTIVATED:
            m_is_connected = false;
            start_dfu_timeout(DFU_DISCONNECT_TIMEOUT_MS);
            break;

        default:
            break;
    }
}

int main(void)
{
    uint32_t ret;

    // Initialize LEDs for visual feedback
    bsp_board_init(BSP_INIT_LEDS);
    bsp_board_led_on(BSP_BOARD_LED_0);  // Indicate bootloader start

    NRF_LOG_INFO("Bootloader starting...");
    NRF_LOG_INFO("Protecting MBR and Bootloader regions");

    nrf_bootloader_mbr_addrs_populate();

    ret = nrf_bootloader_flash_protect(0, MBR_SIZE);
    APP_ERROR_CHECK(ret);
    NRF_LOG_INFO("MBR region protected");

    ret = nrf_bootloader_flash_protect(BOOTLOADER_START_ADDR, BOOTLOADER_SIZE);
    APP_ERROR_CHECK(ret);
    NRF_LOG_INFO("Bootloader region protected");

    // Initialize logging with RTT backend
    ret = NRF_LOG_INIT(nrf_bootloader_dfu_timer_counter_get);
    APP_ERROR_CHECK(ret);
    NRF_LOG_DEFAULT_BACKENDS_INIT();
    NRF_LOG_INFO("Logging initialized");

    // Initialize timers and timeout
    ret = app_timer_init();
    APP_ERROR_CHECK(ret);
    NRF_LOG_INFO("Timers initialized");

    ret = app_timer_create(&m_dfu_timeout_timer, APP_TIMER_MODE_SINGLE_SHOT, dfu_timeout_handler);
    APP_ERROR_CHECK(ret);
    NRF_LOG_INFO("DFU timeout timer created");

    NRF_LOG_INFO("Initializing DFU...");
    ret = nrf_bootloader_init(dfu_observer);
    APP_ERROR_CHECK(ret);
    NRF_LOG_INFO("DFU initialized, waiting for connection...");

    // Start DFU timeout timer
    start_dfu_timeout(DFU_INIT_TIMEOUT_MS);
    NRF_LOG_INFO("DFU timeout timer started (%d ms)", DFU_INIT_TIMEOUT_MS);

    NRF_LOG_ERROR("Unexpected exit");
    NRF_LOG_FINAL_FLUSH();

    APP_ERROR_CHECK_BOOL(false);
}
