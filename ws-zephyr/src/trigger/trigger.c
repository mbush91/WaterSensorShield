#include "trigger.h"
#include "pss_mqtt.h"
#include "main.h"

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(trigger, CONFIG_TRIGGER_LOG_LEVEL);

static const struct gpio_dt_spec water_detect = GPIO_DT_SPEC_GET_OR(DT_NODELABEL(water_detect), gpios, {0});
static const struct gpio_dt_spec pump_trigger = GPIO_DT_SPEC_GET_OR(DT_NODELABEL(pump_running), gpios, {0});
static const struct gpio_dt_spec button1 = GPIO_DT_SPEC_GET(DT_NODELABEL(button0), gpios);
static const struct gpio_dt_spec button2 = GPIO_DT_SPEC_GET(DT_NODELABEL(button1), gpios);

static struct gpio_callback gpio_cb_data;
void gpio_int_cb(const struct device *dev, struct gpio_callback *cb,
		    uint32_t pins)
{
	int32_t val;

	if(BIT(pump_trigger.pin) & pins) {
		val = gpio_pin_get_dt(&pump_trigger);
		if(1 == val) {
			LOG_INF("Pump Running....");
			pss_mqtt_publish(
				"homeassistant/sump/pump",
				"ON",
				MQTT_QOS_1_AT_LEAST_ONCE
			);
		}
		else {
			LOG_INF("Pump Stopped....");
			pss_mqtt_publish(
				"homeassistant/sump/pump",
				"ON",
				MQTT_QOS_1_AT_LEAST_ONCE
			);
		}
	} else if (BIT(water_detect.pin) & pins) {
		val = gpio_pin_get_dt(&water_detect);
		if(1 == val) {
			LOG_INF("Water Detected....");
			pss_mqtt_publish(
				"homeassistant/sump/sensor",
				"ON",
				MQTT_QOS_1_AT_LEAST_ONCE
			);
		}
		else {
			LOG_INF("No Water :D....");
			pss_mqtt_publish(
				"homeassistant/sump/sensor",
				"OFF",
				MQTT_QOS_1_AT_LEAST_ONCE
			);
		}
	} else if (BIT(button1.pin) & pins) {
		LOG_INF("Button1 Pressed");
    main_hearbeat_pub();
	} else if (BIT(button2.pin) & pins) {
		LOG_INF("Button2 Pressed");
	} else {
		LOG_WRN("No pins");
	}
 }

int32_t water_detect_init(void)
{
	int32_t ret;

	if (!gpio_is_ready_dt(&water_detect)) {
		LOG_ERR("Error: water_detect device %s is not ready\n",
		       water_detect.port->name);
		return -1;
	}

	ret = gpio_pin_configure_dt(&water_detect, GPIO_INPUT);
	if (ret != 0) {
		LOG_ERR("Error %d: failed to configure %s pin %d\n",
		       ret, water_detect.port->name, water_detect.pin);
		return ret;
	}

	ret = gpio_pin_interrupt_configure_dt(&water_detect,
					      GPIO_INT_EDGE_BOTH);
	if (ret != 0) {
		LOG_ERR("Error %d: failed to configure interrupt on %s pin %d\n",
			ret, water_detect.port->name, water_detect.pin);
		return ret;
	}

	return 0;
}

int32_t pump_trigger_init(void)
{
	int32_t ret;

	if (!gpio_is_ready_dt(&pump_trigger)) {
		LOG_ERR("Error: pump_trigger device %s is not ready\n",
		       pump_trigger.port->name);
		return -1;
	}

	ret = gpio_pin_configure_dt(&pump_trigger, GPIO_INPUT);
	if (ret != 0) {
		LOG_ERR("Error %d: failed to configure %s pin %d\n",
		       ret, pump_trigger.port->name, pump_trigger.pin);
		return ret;
	}

	ret = gpio_pin_interrupt_configure_dt(&pump_trigger,
					      GPIO_INT_EDGE_BOTH);
	if (ret != 0) {
		LOG_ERR("Error %d: failed to configure interrupt on %s pin %d\n",
			ret, pump_trigger.port->name, pump_trigger.pin);
		return ret;
	}

	return 0;
}

int32_t buttons_trigger_init(void)
{
	int32_t ret;

	if (!gpio_is_ready_dt(&button1)) {
		LOG_ERR("Error: button1 device %s is not ready\n",
		       button1.port->name);
		return -1;
	}

	ret = gpio_pin_configure_dt(&button1, GPIO_INPUT);
	if (ret != 0) {
		LOG_ERR("Error %d: failed to configure %s pin %d\n",
		       ret, button1.port->name, button1.pin);
		return ret;
	}

	ret = gpio_pin_interrupt_configure_dt(&button1,
					      GPIO_INT_EDGE_TO_ACTIVE);
	if (ret != 0) {
		LOG_ERR("Error %d: failed to configure interrupt on %s pin %d\n",
			ret, button1.port->name, button1.pin);
		return ret;
	}

	// BUTTON2 /////////
	if (!gpio_is_ready_dt(&button2)) {
		LOG_ERR("Error: button2 device %s is not ready\n",
		       button2.port->name);
		return -1;
	}

	ret = gpio_pin_configure_dt(&button2, GPIO_INPUT);

	if (ret != 0) {
		LOG_ERR("Error %d: failed to configure %s pin %d\n",
		       ret, button2.port->name, button2.pin);
		return ret;
	}

	ret = gpio_pin_interrupt_configure_dt(&button2,
					      GPIO_INT_EDGE_TO_ACTIVE);
	if (ret != 0) {
		LOG_ERR("Error %d: failed to configure interrupt on %s pin %d\n",
			ret, button2.port->name, button2.pin);
		return ret;
	}

	return 0;
}

int32_t trigger_init(void)
{
	water_detect_init();
	pump_trigger_init();
	buttons_trigger_init();

	gpio_init_callback(&gpio_cb_data, gpio_int_cb, BIT(pump_trigger.pin) | BIT(button1.pin) | BIT(button2.pin) | BIT(water_detect.pin));
	gpio_add_callback(pump_trigger.port, &gpio_cb_data);

	return 0;
}

void trigger_main(void)
{
	// char topic[] = "topic/test";
	// char msg[] = "{\"message\":\"HI\"}";
	// pss_mqtt_publish(&topic,&msg,MQTT_QOS_1_AT_LEAST_ONCE);
}
