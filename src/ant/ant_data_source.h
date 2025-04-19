/**
 * @file ant_data_source.h
 * @brief ANT+ Data Source 
 *
 * This file defines the ANT+ implementation of the data source interface.
 */

#ifndef ANT_DATA_SOURCE_H
#define ANT_DATA_SOURCE_H

#include "includes/data_source.h"

/**
 * @brief Get the ANT+ data source interface
 * 
 * @return data_source_interface_t* Pointer to the ANT+ data source interface
 */
const data_source_interface_t* ant_data_source_get_interface(void);

#endif /* ANT_DATA_SOURCE_H */ 