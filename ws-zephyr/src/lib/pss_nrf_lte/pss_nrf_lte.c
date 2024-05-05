/**
 * @brief Manage LTE network connections and make current status available
 */

#include "pss_nrf_lte.h"

#include <date_time.h>
#include <modem/lte_lc.h>
#include <modem/modem_key_mgmt.h>
#include <modem/nrf_modem_lib.h>
#include <nrf_modem_at.h>
#include <stdint.h>
#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>

LOG_MODULE_REGISTER(pss_nrf_lte, CONFIG_PSS_NRF_LTE_LOG_LEVEL);

#define PSS_NRF_LTE_RESET_WAIT (15 * 60 * 1000)

static bool initialized = false;
static int64_t last_modem_reset = 0;
static pss_nrf_lte_state_t state;
static pss_nrf_lte_evt_handler_t handler;

#if IS_ENABLED(CONFIG_PSS_NRF_LTE_CONNECTION_STATISTICS)
static int64_t stat_start_time = 0;
#endif


static const struct gpio_dt_spec led1 = GPIO_DT_SPEC_GET(DT_NODELABEL(led0), gpios);

static int32_t init_led(void)
{
  int32_t ret;
  if (!gpio_is_ready_dt(&led1))
  {
    return 0;
  }

  ret = gpio_pin_configure_dt(&led1, GPIO_OUTPUT_ACTIVE);
  if (ret < 0)
  {
    return 0;
  }

  gpio_pin_set_dt(&led1,0);

  return 0;
}

pss_nrf_lte_state_t pss_nrf_lte_get_state(void)
{
    return state;
}

/**
 * @brief Read the current time and daylight saving time.
 */
static void modem_time_check(void)
{
    char buf[64] = { 0 };
    int err;

    for (int i = 0; i < 10; i++) {
        err = nrf_modem_at_cmd(buf, sizeof(buf), "AT%%CCLK?");
        if (!err) {
            break;
        } else {
            (void)k_msleep(100);
        }
    }

    if (err) {
        LOG_WRN("AT Clock Command Error %d", err);
    } else {
        LOG_INF("Network time %s", buf);
    }

    err = nrf_modem_at_cmd(buf, sizeof(buf), "AT+CNUM");
    if (err) {
        LOG_WRN("Could not get number %d", err);
    } else {
        LOG_INF("Phone Number: %s", buf);
    }
}

static void update_state(pss_nrf_lte_state_t new_state)
{
    if(new_state == LTE_STATE_CONNECTED) {
      gpio_pin_set_dt(&led1,1);
    } else {
      gpio_pin_set_dt(&led1,0);
    }

    if (state != new_state) {
        LOG_INF("Changing LTE state to: %s",
                (new_state == LTE_STATE_CONNECTED)
                        ? "LTE_STATE_CONNECTED"
                        : ((new_state == LTE_STATE_DISCONNECTED) ? "LTE_STATE_DISCONNECTED"
                                                                 : "LTE_STATE_ERROR"));

        if (handler) {
            handler(LTE_STATE_DISCONNECTED);
        }
    }
    state = new_state;
}

/**
 * @brief Callback for handling LTE events and updating the state.
 * Note the event handling here is not exhaustive; some events are not covered
 *
 * @param evt The event type and its payload
 */
static void lte_handler_cb(const struct lte_lc_evt* const evt)
{
    switch (evt->type) {
        case LTE_LC_EVT_NW_REG_STATUS:
            switch (evt->nw_reg_status) {
                case LTE_LC_NW_REG_NOT_REGISTERED:
                    LOG_WRN("Network registration status: Not registered");
                    update_state(LTE_STATE_DISCONNECTED);
                    (void)k_msleep(30000); // wait 30 seconds before restarting LTE
                    (void)pss_nrf_lte_connect();
                    break;
                case LTE_LC_NW_REG_SEARCHING:
                    LOG_DBG("Network registration status: searching");
                    break;
                case LTE_LC_NW_REG_UNKNOWN:
                    LOG_DBG("Network registration status: Unknown");
                    update_state(LTE_STATE_DISCONNECTED);
                    (void)pss_nrf_lte_connect();
                    break;
                case LTE_LC_NW_REG_REGISTRATION_DENIED:
                case LTE_LC_NW_REG_UICC_FAIL:
                    LOG_DBG("Network registration status: %s",
                            evt->nw_reg_status == LTE_LC_NW_REG_REGISTRATION_DENIED
                                    ? "Registration denied"
                                    : "UICC failure");
                    state = LTE_STATE_ERROR;
                    if (handler) {
                        handler(LTE_STATE_ERROR);
                    }
                    break;
                case LTE_LC_NW_REG_REGISTERED_HOME:
                case LTE_LC_NW_REG_REGISTERED_ROAMING:
                    LOG_DBG("Network registration status: %s",
                            evt->nw_reg_status == LTE_LC_NW_REG_REGISTERED_HOME
                                    ? "Connected - home network"
                                    : "Connected - roaming");
                    modem_time_check();
                    (void)k_msleep(15000); // Wait 15 seconds to notify system.
                    update_state(LTE_STATE_CONNECTED);
#if IS_ENABLED(CONFIG_PSS_NRF_LTE_CONNECTION_STATISTICS)
                    char en_stat_buf[128];
                    int err = nrf_modem_at_cmd(en_stat_buf,
                                               sizeof(en_stat_buf),
                                               "AT%%XCONNSTAT=1");
                    if (err) {
                        LOG_WRN("AT%%CONNSTAT Command Error %d", err);
                    } else {
                        LOG_DBG("Connectivity statistic tracking enable: %s", en_stat_buf);
                        stat_start_time = k_uptime_get();
                    }
#endif
            }
            break;
        case LTE_LC_EVT_PSM_UPDATE:
            LOG_DBG("PSM parameter update: TAU: %d, Active time: %d",
                    evt->psm_cfg.tau,
                    evt->psm_cfg.active_time);
            break;
        case LTE_LC_EVT_EDRX_UPDATE: {
            char log_buf[60];
            ssize_t len;

            len = snprintf(log_buf,
                           sizeof(log_buf),
                           "eDRX parameter update: eDRX: %f, PTW: %f",
                           evt->edrx_cfg.edrx,
                           evt->edrx_cfg.ptw);
            if (len > 0) {
                LOG_INF("%s", log_buf);
            }
            break;
        }
        case LTE_LC_EVT_RRC_UPDATE:
            LOG_DBG("RRC mode: %s",
                    evt->rrc_mode == LTE_LC_RRC_MODE_CONNECTED ? "Connected" : "Idle");
            break;
        case LTE_LC_EVT_CELL_UPDATE:
            LOG_DBG("LTE cell changed: Cell ID: %d, Tracking area: %d",
                    evt->cell.id,
                    evt->cell.tac);
            break;
        case LTE_LC_EVT_LTE_MODE_UPDATE:
            LOG_DBG("LTE_LC_EVT_LTE_MODE_UPDATE");
            break;
        case LTE_LC_EVT_TAU_PRE_WARNING:
            LOG_DBG("LTE_LC_EVT_TAU_PRE_WARNING");
            break;
        case LTE_LC_EVT_NEIGHBOR_CELL_MEAS:
            LOG_DBG("LTE_LC_EVT_NEIGHBOR_CELL_MEAS");
            break;
        case LTE_LC_EVT_MODEM_SLEEP_EXIT_PRE_WARNING:
            LOG_DBG("LTE_LC_EVT_MODEM_SLEEP_EXIT_PRE_WARNING");
            break;
        case LTE_LC_EVT_MODEM_SLEEP_EXIT:
            LOG_DBG("LTE_LC_EVT_MODEM_SLEEP_EXIT");
            break;
        case LTE_LC_EVT_MODEM_SLEEP_ENTER:
            LOG_DBG("LTE_LC_EVT_MODEM_SLEEP_ENTER");
            break;
        case LTE_LC_EVT_MODEM_EVENT:
            LOG_DBG("LTE_LC_EVT_MODEM_EVENT");
            break;
        default:
            break;
    }
}


int pss_nrf_lte_init(pss_nrf_lte_evt_handler_t h)
{
    handler = h;
    state = LTE_STATE_DISCONNECTED;
    /* Initialize libraries and hardware */
    /* Init modem */
    int err = nrf_modem_lib_init();
    if ((err != 0) && (err != -1)) {
        LOG_ERR("Failed to initialize nrf_modem_lib library: 0x%X", err);
        return -EFAULT;
    } else {
        LOG_INF("nrf_modem_lib initialized.");
    }

    init_led();

    return 0;
}

int32_t pss_nrf_lte_connect(void)
{
    int32_t err = 0;
    if (!initialized) {
        LOG_INF("Connecting LTE...");
        err = lte_lc_init_and_connect_async(lte_handler_cb);
        if (err) {
            LOG_ERR("Failed to init modem, error: %d", err);
        } else {
            initialized = true;
        }
    } else {
        LOG_DBG("Already initialized");
    }

    return err;
}

bool pss_nrf_lte_connected(void)
{
    return state == LTE_STATE_CONNECTED;
}

#if IS_ENABLED(CONFIG_PSS_NRF_LTE_CONNECTION_STATISTICS)
static int32_t extract_XCONNSTAT(const char* buf,
                                 int32_t* sms_tx,
                                 int32_t* sms_rx,
                                 int32_t* data_tx,
                                 int32_t* data_rx,
                                 int32_t* packet_max,
                                 int32_t* packet_avg)
{
    int32_t count, ret = 0;
    if (buf == NULL || sms_tx == NULL || sms_rx == NULL || data_tx == NULL || data_rx == NULL
        || packet_max == NULL || packet_avg == NULL) {
        // Handle invalid input pointers (return or log error, if desired)
        return -EIO;
    }

    // Use sscanf to parse the values
    count = sscanf(buf,
                   "%%XCONNSTAT: %d,%d,%d,%d,%d,%d",
                   sms_tx,
                   sms_rx,
                   data_tx,
                   data_rx,
                   packet_max,
                   packet_avg);

    if (count != 6) {
        ret = -1;
    }

    return ret;
}
#endif

int32_t pss_nrf_lte_deinit(void)
{
    int64_t tm;

    tm = k_uptime_get();

    if ((tm - last_modem_reset) > PSS_NRF_LTE_RESET_WAIT) {
        LOG_WRN("Deinitializing Modem");
        update_state(LTE_STATE_ERROR);
        initialized = false;
        last_modem_reset = tm;
        return lte_lc_deinit();
    } else {
        return -EBUSY;
    }
}

void pss_nrf_lte_print_connection_stats(void)
{
#if IS_ENABLED(CONFIG_PSS_NRF_LTE_CONNECTION_STATISTICS)
    int32_t sms_tx, sms_rx, data_tx = 0, data_rx = 0, packet_max, packet_avg, total_data;
    char en_stat_buf[128];
    int err;
    err = nrf_modem_at_cmd(en_stat_buf, sizeof(en_stat_buf), "AT%%XCONNSTAT?");

    if (0 == err) {
        err = extract_XCONNSTAT(en_stat_buf,
                                &sms_tx,
                                &sms_rx,
                                &data_tx,
                                &data_rx,
                                &packet_max,
                                &packet_avg);
    }

    if (0 == err) {
        total_data = data_tx + data_rx;
        int32_t elapsed = (int32_t)((k_uptime_get() - stat_start_time) / 1000);
        LOG_INF("SMS TX: %d, SMS RX: %d, Data TX: %d, Data RX: %d, Packet Max: %d, Packet Avg: %d, Total KB: %d Over %d seconds.",
                sms_tx,
                sms_rx,
                data_tx,
                data_rx,
                packet_max,
                packet_avg,
                total_data,
                elapsed);
    }
#endif
}

/**
 * @brief Get a epoch timestamp of "now", in milliseconds.
 *
 * @retval The ms since 01/01/1970, else 0
 */
int64_t pss_nrf_lte_get_time(void)
{
    // Holds time in ms
    static int64_t time_now;

    // Confirm the library is working
    bool isValid = date_time_is_valid();
    if (!isValid) {
        return 0;
    }
    // Get the current local time.
    int ret = date_time_now(&time_now);
    if (ret != 0) {
        return 0;
    }
    return time_now;
}
