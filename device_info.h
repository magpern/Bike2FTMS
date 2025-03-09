#ifndef DEVICE_INFO_H__
#define DEVICE_INFO_H__

#include <stdint.h>
#include <stddef.h>

#ifndef FIRMWARE_VERSION
// Fallback if not set in build
#define FIRMWARE_VERSION "UNKNOWN"  
#endif

/**@brief Function to get the device's unique serial number. */
void get_serial_number(char *serial_str, size_t len);

/**@brief Function to get the hardware revision. */
void get_hardware_revision(char *hw_rev_str, size_t len);

void get_firmware_version(char *version_str, size_t len);

#endif // DEVICE_INFO_H__
