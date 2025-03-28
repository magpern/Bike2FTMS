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
#include "app_timer.h"  // ✅ Add this

#define DFU_NO_CONN_TIMEOUT_MS      30000  // Timeout when no central connects
#define DFU_DISCONNECT_TIMEOUT_MS   10000  // Timeout after disconnect if DFU not started

APP_TIMER_DEF(m_dfu_timeout_timer);  // ✅ Timer instance

static void dfu_timeout_handler(void * p_context)
{
    NRF_LOG_WARNING("⏱️ DFU timeout, rebooting device.");
    NRF_LOG_FLUSH();
    NVIC_SystemReset();
}

static void on_error(void)
{
    NRF_LOG_FINAL_FLUSH();

#if NRF_MODULE_ENABLED(NRF_LOG_BACKEND_RTT)
    // To allow the buffer to be flushed by the host.
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
    NRF_LOG_ERROR("Received a fault! id: 0x%08x, pc: 0x%08x, info: 0x%08x", id, pc, info);
    on_error();
}

void app_error_handler_bare(uint32_t error_code)
{
    NRF_LOG_ERROR("Received an error: 0x%08x!", error_code);
    on_error();
}

/**
 * @brief Function notifies certain events in DFU process.
 */
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

            // ✅ Start initial timeout (no one connects)
            app_timer_start(m_dfu_timeout_timer, APP_TIMER_TICKS(DFU_NO_CONN_TIMEOUT_MS), NULL);
            break;

        case NRF_DFU_EVT_TRANSPORT_ACTIVATED:
            bsp_board_led_off(BSP_BOARD_LED_1);
            bsp_board_led_on(BSP_BOARD_LED_2);

            // ✅ Stop timeout when connected
            app_timer_stop(m_dfu_timeout_timer);
            break;

        case NRF_DFU_EVT_DFU_STARTED:
            // ✅ DFU transfer has begun — cancel timeout
            app_timer_stop(m_dfu_timeout_timer);
            break;

        default:
            break;
    }
}

/**@brief Function for application main entry. */
int main(void)
{
    uint32_t ret_val;

    nrf_bootloader_mbr_addrs_populate();

    ret_val = nrf_bootloader_flash_protect(0, MBR_SIZE);
    APP_ERROR_CHECK(ret_val);
    ret_val = nrf_bootloader_flash_protect(BOOTLOADER_START_ADDR, BOOTLOADER_SIZE);
    APP_ERROR_CHECK(ret_val);

    (void) NRF_LOG_INIT(nrf_bootloader_dfu_timer_counter_get);
    NRF_LOG_DEFAULT_BACKENDS_INIT();

    NRF_LOG_INFO("Inside main");

    // ✅ Initialize app_timer before bootloader
    ret_val = app_timer_init();
    APP_ERROR_CHECK(ret_val);

    // ✅ Create the timeout timer
    ret_val = app_timer_create(&m_dfu_timeout_timer, APP_TIMER_MODE_SINGLE_SHOT, dfu_timeout_handler);
    APP_ERROR_CHECK(ret_val);

    ret_val = nrf_bootloader_init(dfu_observer);
    APP_ERROR_CHECK(ret_val);

    NRF_LOG_FLUSH();

    NRF_LOG_ERROR("After main, should never be reached.");
    NRF_LOG_FLUSH();

    APP_ERROR_CHECK_BOOL(false);
}
