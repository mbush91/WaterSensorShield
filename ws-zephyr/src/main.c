#include "battery.h"
#include "scheduler.h"
#include "trigger.h"
#include "pss_nrf_lte.h"
#include "pss_mqtt.h"

#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>

#define TEN_SEC_IN_12HOURS (4320)

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

static const struct gpio_dt_spec led4 = GPIO_DT_SPEC_GET(DT_NODELABEL(led3), gpios);

static int32_t init_led(void)
{
  int32_t ret;
  if (!gpio_is_ready_dt(&led4))
  {
    return 0;
  }

  ret = gpio_pin_configure_dt(&led4, GPIO_OUTPUT_ACTIVE);
  if (ret < 0)
  {
    return 0;
  }

  gpio_pin_set_dt(&led4, 1);

  return 0;
}

void main_main_blink(void)
{
  gpio_pin_toggle_dt(&led4);
}

void main_hearbeat_pub(void)
{
  battery_info_t batt;
  char batt_v[10];

  battery_get_last_read(&batt);
  snprintf(batt_v, sizeof(batt_v), "%.2f", ((float)batt.lvl_mV / 1000.0f));

  pss_mqtt_publish(
          "homeassistant/sump/availability",
          "online",
          MQTT_QOS_1_AT_LEAST_ONCE);
  pss_mqtt_publish(
      "homeassistant/sump/batt",
      batt_v,
      MQTT_QOS_1_AT_LEAST_ONCE);
}

void main_main_hearbeat(void)
{
  static int32_t loop_cnt = 0;
  battery_info_t batt;
  char batt_v[10];

  battery_get_last_read(&batt);
  snprintf(batt_v, sizeof(batt_v), "%.2f", ((float)batt.lvl_mV / 1000.0f));
  LOG_INF("Battery: %sV", batt_v);

  if (loop_cnt == 0)
  {
    if (pss_mqtt_connected())
    {
      main_hearbeat_pub();
      loop_cnt++;
    }
  }
  else
  {
    loop_cnt++;
    if (loop_cnt > TEN_SEC_IN_12HOURS)
    {
      loop_cnt = 0;
    }
  }
}

int main(void)
{
  init_led();
  battery_init();
  battery_main();

  trigger_init();
  if (pss_nrf_lte_init(NULL))
  {
    LOG_ERR("Could not init LTE");
  }
  pss_mqtt_provision();
  pss_nrf_lte_connect();
  pss_mqtt_init();

  scheduler_init();
}
