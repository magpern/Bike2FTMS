#ifndef ANT_SCANNER_H
#define ANT_SCANNER_H

#include <stdint.h>

/**@brief Initialize and open the ANT scanning channel.
 * 
 * This function will:
 *   - Configure the ANT channel (wildcard or partial wildcard).
 *   - Register the necessary SoftDevice observer for ANT events.
 *   - Open the channel for RX scanning.
 */
void ant_scanner_init(void);

#endif // ANT_SCANNER_H
