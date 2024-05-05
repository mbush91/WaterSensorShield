#ifndef APPLICATION_BATTERY_H_
#define APPLICATION_BATTERY_H_

#include <stdbool.h>
#include <stdint.h>

/**
 * @brief A struct containing batter read info
 */
typedef struct {
    uint8_t lvl_percent; // Percentage representation of the battery level
    uint16_t lvl_mV;     // Battery level in millivolts
} battery_info_t;

/**
 * @brief Initializes the battery module
 * @details Prepares the battery hardware/module for use
 *
 * @return int 0 for success, non-zero for errors
 */
int battery_init(void);

/**
 * @brief Main function to handle battery operations
 * @details Intended to be called by the scheduler or main loop
 */
void battery_main(void);

/**
 * @brief Fetches the last recorded information about the battery
 * @details Provides both the percentage and millivolt representation of the battery level
 *
 * @param batt_info Pointer to a battery_info_t structure to store the retrieved information
 * @return int32_t 0 for success, non-zero for errors
 */
int32_t battery_get_last_read(battery_info_t* batt_info);

// Ensure that the defined battery voltage limits are consistent
#if ((CONFIG_BATTERY_LIB_MAX_VOLTAGE < CONFIG_BATTERY_LIB_LOW_VOLTAGE) \
     || (CONFIG_BATTERY_LIB_LOW_VOLTAGE < CONFIG_BATTERY_LIB_MIN_OPERATING_VOLTAGE))
#error "Check battery voltage configuration"
#endif

#endif /* APPLICATION_BATTERY_H_ */
