/**
 * @file battery_monitor.h
 * @brief Battery monitoring module for nRF52840 using nRF5 SDK
 *
 * This module provides functions to initialize battery monitoring,
 * measure battery voltage using the SAADC, and get the battery level
 * as a percentage.
 */

 #ifndef BATTERY_MONITOR_H__
 #define BATTERY_MONITOR_H__
 
 #include <stdint.h>
 
 /**
  * @brief Initialize the battery monitoring module
  *
  * This function initializes the SAADC for battery voltage measurement
  * and sets up a timer for periodic measurements.
  */
 void battery_monitoring_init(void);
 
 /**
  * @brief Measure battery level
  *
  * This function triggers an ADC conversion to measure the battery voltage.
  * The result is processed in the SAADC event handler.
  */
 void battery_level_measure(void);
 
 /**
  * @brief Get the current battery level percentage
  *
  * @return Battery level as percentage (0-100)
  */
 uint8_t battery_level_get(void);
 uint16_t get_battery_voltage(void);

 /**
  * @brief Uninitialize the battery monitoring
  *
  * This function stops the battery measurement timer and uninitializes
  * the SAADC peripheral.
  */
 void battery_monitoring_uninit(void);
 
 #endif // BATTERY_MONITOR_H__