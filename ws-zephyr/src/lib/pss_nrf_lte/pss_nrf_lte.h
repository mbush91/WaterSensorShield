#ifndef PSS_NRF_LTE_H
#define PSS_NRF_LTE_H
#include <stdbool.h>
#include <stdint.h>

typedef enum {
    LTE_STATE_CONNECTED,
    LTE_STATE_ERROR,
    LTE_STATE_DISCONNECTED,
} pss_nrf_lte_state_t;

typedef void (*pss_nrf_lte_evt_handler_t)(const pss_nrf_lte_state_t evt);

/**
 * @brief Initialize the nRF LTE module, has optional callback
 *
 * @param handler Callback for LTE state change, NULL if not needed
 * @return 0 if successful, -EFAULT otherwise
 */
int pss_nrf_lte_init(pss_nrf_lte_evt_handler_t handler);

/**
 * @brief Initialize the LTE library, configure the modem,
 * and connect to LTE network.
 *
 * @return 0 if connected successfully, else fatal error is thrown
 */
int32_t pss_nrf_lte_connect(void);

/**
 * @brief Return LTE network connection status
 *
 * @return true Connected to the LTE network
 * @return false  Not connected to LTE network
 */
bool pss_nrf_lte_connected(void);

/**
 * @brief Disconnect and deinit modem
 *
 * @retval 0 if successful.
 * @retval -EFAULT if an AT command failed.
*/
int32_t pss_nrf_lte_deinit(void);

/**
 * @brief Print the connection stats
*/
void pss_nrf_lte_print_connection_stats(void);

/**
 * @brief Set get the LTE time
 *
 * @return int64_t The time in milliseconds
*/
int64_t pss_nrf_lte_get_time(void);

/**
 * @brief Return the current lte state
 *
 * @retval LTE_STATE_CONNECTED Network is connected
 * @retval LTE_STATE_DISCONNECTED Network is not connected
 * @retval LTE_STATE_ERROR A problem reaching the network or registering
 */
pss_nrf_lte_state_t pss_nrf_lte_get_state(void);
#endif // PSS_NRF_LTE_H
