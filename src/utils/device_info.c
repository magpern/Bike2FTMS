#include "device_info.h"
#include "nrf.h"
#include <stdio.h>
#include <inttypes.h>  // ✅ Include for PRIX32

/**@brief Function to get the chip's unique serial number (as a string). */
void get_serial_number(char *serial_str, size_t len)
{
    uint32_t device_id_high = NRF_FICR->DEVICEID[1]; // Upper 32 bits
    uint32_t device_id_low  = NRF_FICR->DEVICEID[0]; // Lower 32 bits

    // ✅ Use PRIX32 to ensure correct formatting across platforms
    snprintf(serial_str, len, "%08" PRIX32 "-%08" PRIX32, device_id_high, device_id_low);
}

/**@brief Function to get the hardware revision. */
void get_hardware_revision(char *hw_rev_str, size_t len)
{
    if (NRF_UICR->CUSTOMER[0] != 0xFFFFFFFF) {
        snprintf(hw_rev_str, len, "HW Rev %08" PRIX32, NRF_UICR->CUSTOMER[0]);
    } else {
        snprintf(hw_rev_str, len, "Rev A");  // Default if no info is set
    }
}

/**@brief Function to get the firmware version. */
void get_firmware_version(char *version_str, size_t len)
{
    snprintf(version_str, len, "FW %s", FIRMWARE_VERSION);
}