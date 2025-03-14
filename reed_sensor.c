#include "reed_sensor.h"
#include "app_error.h"
#include "nrf_log.h"

static reed_sensor_callback_t sensor_callback = NULL;

void reed_switch_handler(nrf_drv_gpiote_pin_t pin, nrf_gpiote_polarity_t action)
{
    NRF_LOG_INFO("🚴 Flywheel movement detected!");

    if (sensor_callback != NULL)
    {
        sensor_callback();  // Call function from main.c
    }
}

void reed_sensor_init(reed_sensor_callback_t callback)
{
    ret_code_t err_code;

    sensor_callback = callback;  // Store the function from main.c

    err_code = nrf_drv_gpiote_init();
    APP_ERROR_CHECK(err_code);

    // ✅ Keep it disabled by default to save power
    nrf_drv_gpiote_in_event_disable(REED_SWITCH_PIN);
}

void reed_sensor_enable(void)
{
    nrf_drv_gpiote_in_config_t config = GPIOTE_CONFIG_IN_SENSE_HITOLO(true);
    config.pull = NRF_GPIO_PIN_PULLUP;

    ret_code_t err_code = nrf_drv_gpiote_in_init(REED_SWITCH_PIN, &config, reed_switch_handler);
    APP_ERROR_CHECK(err_code);
    
    nrf_drv_gpiote_in_event_enable(REED_SWITCH_PIN, true);

    NRF_LOG_INFO("✅ Reed sensor enabled for wake-up.");
}

void reed_sensor_disable(void)
{
    nrf_drv_gpiote_in_event_disable(REED_SWITCH_PIN);
    nrf_drv_gpiote_in_uninit(REED_SWITCH_PIN);

    NRF_LOG_INFO("🚫 Reed sensor disabled to save power.");
}
