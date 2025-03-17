#include <stdbool.h>
#include <stdint.h>
#include "nrf_drv_saadc.h"
#include "nrf_saadc.h"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
#include "app_error.h"
#include "app_timer.h"
#include "battery_measurement.h"

// 🛠 Configuration Constants
#define BATTERY_LEVEL_MEAS_INTERVAL     APP_TIMER_TICKS(60000)  // Battery check every 30s
#define ADC_REF_VOLTAGE_IN_MILLIVOLTS   600   // Internal reference voltage (600mV)
#define ADC_PRE_SCALING_COMPENSATION    6     // 1/6 prescaler for up to 3.6V
#define BATTERY_MAX_MILLIVOLTS 3000  // ✅ AA Batteries Full (3.0V)
#define BATTERY_MIN_MILLIVOLTS 2000  // ✅ AA Batteries Empty (2.0V)

// 🛠 ADC Channel Selection (Measure VDD)
#define BATTERY_ADC_CHANNEL             0

// 🔄 Battery measurement timer
APP_TIMER_DEF(m_battery_timer_id);

// 🔋 Battery State
static uint8_t m_battery_level_percent = 100;
static int32_t last_battery_voltage = BATTERY_MIN_MILLIVOLTS;
static nrf_saadc_value_t adc_buf[1];  // ADC Buffer
static bool adc_initialized = false;  // Prevent reinitialization

// 🛠 Function Declarations
static void battery_level_meas_timeout_handler(void *p_context);
static void saadc_event_handler(nrf_drv_saadc_evt_t const *p_event);
static void saadc_init(void);

/**
 * @brief Initialize Battery Monitoring
 */
void battery_monitoring_init(void)
{
    ret_code_t err_code;

    // ✅ Initialize ADC
    saadc_init();

    // ✅ Create Timer for Periodic Measurement
    err_code = app_timer_create(&m_battery_timer_id,
                                APP_TIMER_MODE_REPEATED,
                                battery_level_meas_timeout_handler);
    APP_ERROR_CHECK(err_code);

    // ✅ Start Battery Measurement Timer (Every BATTERY_LEVEL_MEAS_INTERVAL s)
    err_code = app_timer_start(m_battery_timer_id, BATTERY_LEVEL_MEAS_INTERVAL, NULL);
    APP_ERROR_CHECK(err_code);

    // ✅ Perform Initial Measurement
    battery_level_measure();

    NRF_LOG_INFO("🔋 Battery Monitoring Initialized.");
}

/**
 * @brief Timer Handler for Battery Measurement
 */
static void battery_level_meas_timeout_handler(void *p_context)
{
    UNUSED_PARAMETER(p_context);
    battery_level_measure();
}

/**
 * @brief Initialize SAADC for Battery Voltage Measurement
 */
static void saadc_init(void)
{
    if (adc_initialized) return;  // ✅ Prevent Reinitialization

    ret_code_t err_code;

    // ✅ Initialize SAADC with Default Settings
    err_code = nrf_drv_saadc_init(NULL, saadc_event_handler);
    APP_ERROR_CHECK(err_code);

    // ✅ Configure ADC Channel for VDD Measurement
    nrf_saadc_channel_config_t channel_config =
        NRF_DRV_SAADC_DEFAULT_CHANNEL_CONFIG_SE(NRF_SAADC_INPUT_VDD);

    err_code = nrf_drv_saadc_channel_init(BATTERY_ADC_CHANNEL, &channel_config);
    APP_ERROR_CHECK(err_code);

    // ✅ Set ADC Buffer for Sampling
    err_code = nrf_drv_saadc_buffer_convert(adc_buf, 1);
    APP_ERROR_CHECK(err_code);

    adc_initialized = true;
}

/**
 * @brief SAADC Event Handler
 */
/**
 * @brief SAADC Event Handler
 */
static void saadc_event_handler(nrf_drv_saadc_evt_t const *p_event)
{
    if (p_event->type == NRF_DRV_SAADC_EVT_DONE)
    {
        nrf_saadc_value_t adc_result = p_event->data.done.p_buffer[0];


        // ✅ Convert ADC Reading to Millivolts & Store it
        last_battery_voltage = (ADC_REF_VOLTAGE_IN_MILLIVOLTS * adc_result * ADC_PRE_SCALING_COMPENSATION) / 1024;

        // ✅ Convert Voltage to Battery Percentage (2000mV = 0%, 3000mV = 100%)
        if (last_battery_voltage >= BATTERY_MAX_MILLIVOLTS) {
            m_battery_level_percent = 100;
        }
        else if (last_battery_voltage <= BATTERY_MIN_MILLIVOLTS) {
            m_battery_level_percent = 0;
        }
        else {
            m_battery_level_percent = (uint8_t)(((last_battery_voltage - BATTERY_MIN_MILLIVOLTS) * 100) /
                                                (BATTERY_MAX_MILLIVOLTS - BATTERY_MIN_MILLIVOLTS));
        }

        NRF_LOG_INFO("🔋 Battery Voltage: %d mV, Level: %d%%", last_battery_voltage, m_battery_level_percent);

        // ✅ Prepare Buffer for Next ADC Sample
        ret_code_t err_code = nrf_drv_saadc_buffer_convert(adc_buf, 1);
        APP_ERROR_CHECK(err_code);
    }
}


/**
 * @brief Measure Battery Level
 */
void battery_level_measure(void)
{
    if (!adc_initialized) {
        saadc_init();
    }

    // ✅ Trigger ADC Sampling
    ret_code_t err_code = nrf_drv_saadc_sample();
    APP_ERROR_CHECK(err_code);
}

/**
 * @brief Get the Current Battery Level Percentage
 * @return Battery Level (0-100%)
 */
uint8_t battery_level_get(void)
{
    return m_battery_level_percent;
}

/**
 * @brief Get the Current Battery Level Percentage
 * @return Battery Level (0-100%)
 */
uint16_t get_battery_voltage(void)
{
    return (uint16_t) last_battery_voltage;  // ✅ Return last measured voltage
}

/**
 * @brief Uninitialize Battery Monitoring
 */
void battery_monitoring_uninit(void)
{
    // ✅ Stop Timer
    app_timer_stop(m_battery_timer_id);

    // ✅ Uninitialize SAADC
    if (adc_initialized) {
        nrf_drv_saadc_uninit();
        adc_initialized = false;
    }
}
