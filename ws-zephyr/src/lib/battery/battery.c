/*
 * Copyright (c) 2018-2019 Peter Bigot Consulting, LLC
 * Copyright (c) 2019-2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "battery.h"

LOG_MODULE_REGISTER(battery, CONFIG_BATTERY_LOG_LEVEL);

#define ZEPHYR_USER DT_PATH(zephyr_user)

#define GPIO_EN_ON  0 //lint !e750 may not be used
#define GPIO_EN_OFF 1 //lint !e750 may not be used

/* local variables */
#if IS_ENABLED(CONFIG_BATTERY_LIB_GPIO_EN)
const struct gpio_dt_spec batt_en_dt = GPIO_DT_SPEC_GET(DT_NODELABEL(batt_en), gpios);
#endif

#define BATTERY_ADC_GAIN ADC_GAIN_1_3
#define OUTPUT_RES       (CONFIG_BATTERY_LIB_DIV_R2)
#define FULL_DIV_RES     (CONFIG_BATTERY_LIB_DIV_R2 + CONFIG_BATTERY_LIB_DIV_R1)

#if (FULL_DIV_RES > INT16_MAX)
#error "Resistor Network should be less than INT16_MAX"
#endif

struct io_channel_config {
    uint8_t channel;
};

const struct device* const adc = DEVICE_DT_GET(DT_IO_CHANNELS_CTLR(ZEPHYR_USER));
static struct adc_channel_cfg adc_cfg;
static struct adc_sequence adc_seq;
static int16_t raw_measured;
static struct io_channel_config io_channel;
static battery_info_t last_value;
static bool battery_ok;

K_MUTEX_DEFINE(last_value_mutex);

uint8_t battery_level_pptt(uint32_t batt_mV);

int32_t battery_init(void)
{
    int32_t rc;

    if (!device_is_ready(adc)) {
        LOG_ERR("ADC device is not ready %s", adc->name);
        return -ENOENT;
    };

    last_value.lvl_mV = 0;
    last_value.lvl_percent = 0;

    io_channel.channel = DT_IO_CHANNELS_INPUT(ZEPHYR_USER);


    adc_seq = (struct adc_sequence){
        .channels = BIT(0),
        .buffer = &raw_measured,
        .buffer_size = sizeof(raw_measured),
        .oversampling = 4,
        .calibrate = true,
        .resolution = 14,
    };

    adc_cfg = (struct adc_channel_cfg){
        .gain = BATTERY_ADC_GAIN,
        .reference = ADC_REF_INTERNAL,
        .acquisition_time = ADC_ACQ_TIME(ADC_ACQ_TIME_MICROSECONDS, 40),
        .input_positive = SAADC_CH_PSELP_PSELP_AnalogInput0 + io_channel.channel,
    };


    rc = adc_channel_setup(adc, &adc_cfg);
    LOG_DBG("Setup AIN%u got %d", io_channel.channel, rc);

#if IS_ENABLED(CONFIG_BATTERY_LIB_GPIO_EN)
    if (gpio_is_ready_dt(&batt_en_dt) != false) {
        rc = gpio_pin_configure_dt(&batt_en_dt, GPIO_OUTPUT_ACTIVE);
        gpio_pin_set_dt(&batt_en_dt, GPIO_EN_OFF);
        LOG_DBG("Battery Sense Enable GPIO ready");
    } else {
        LOG_DBG("Battery Sense Enable GPIO not ready");
        rc = -1;
    }
#endif

    battery_ok = (rc == 0);

    LOG_INF("Battery setup: %d %d", rc, battery_ok);

    if (rc == 0) {
        battery_main();
    }

    return rc;
}

int32_t battery_sample(void)
{
    int32_t rc = -ENOENT;

    if (battery_ok) {
#if IS_ENABLED(CONFIG_BATTERY_LIB_GPIO_EN)
        gpio_pin_set_dt(&batt_en_dt, GPIO_EN_ON);
        k_sleep(K_MSEC(1));
#endif

        rc = adc_read(adc, &adc_seq);
        adc_seq.calibrate = false;
        if (rc == 0) {
            int32_t val = raw_measured;

            if (0
                != adc_raw_to_millivolts(adc_ref_internal(adc),
                                         adc_cfg.gain,
                                         adc_seq.resolution,
                                         &val)) {
                LOG_ERR("adc_raw_to_millivolts failed");
            }

            // TODO: SCALE FOR SOME UNKNOWN RESON
            val = (val * 1069) / 1000;


            LOG_DBG("raw_measured to mv: %d -> %d", raw_measured, val);

            if (OUTPUT_RES != 0) { //lint !e774 !e506 set by proj.conf
                rc = (val * (int32_t)FULL_DIV_RES / OUTPUT_RES);
                LOG_DBG("Calc mV: %d", rc);
            } else {
                rc = val;
            }

            if (rc < 0) {
                rc = 0;
            }
        }
#if IS_ENABLED(CONFIG_BATTERY_LIB_GPIO_EN)
        gpio_pin_set_dt(&batt_en_dt, GPIO_EN_OFF);
#endif
    }

    return rc;
}

int32_t battery_get_last_read(battery_info_t* batt_info)
{
    int32_t ret = 0;

    if (0 == last_value.lvl_mV) {
        ret = -1;
    } else {
        (void)k_mutex_lock(&last_value_mutex, K_FOREVER);
        memcpy(batt_info, &last_value, sizeof(battery_info_t));
        (void)k_mutex_unlock(&last_value_mutex);
    }

    return ret;
}

void battery_main(void)
{
    int32_t batt_mV;
    uint8_t batt_percentage;

    batt_mV = battery_sample();

    if (batt_mV >= CONFIG_BATTERY_LIB_PLAUS_LOW_VOLTAGE) {
        batt_percentage = battery_level_pptt((uint32_t)batt_mV);

        (void)k_mutex_lock(&last_value_mutex, K_FOREVER);
        last_value.lvl_mV = (uint16_t)batt_mV;
        last_value.lvl_percent = batt_percentage;
        (void)k_mutex_unlock(&last_value_mutex);
    } else {
        (void)k_mutex_lock(&last_value_mutex, K_FOREVER);
        last_value.lvl_mV = 0;
        last_value.lvl_percent = 0;
        (void)k_mutex_unlock(&last_value_mutex);
    }
}

uint8_t battery_level_pptt(uint32_t batt_mV)
{
    uint32_t mV_max, mV_min, upper, lower, numerator, denominator, val = 100;
    const uint32_t multiplier = 100;
    uint8_t ret;

    if (batt_mV > CONFIG_BATTERY_LIB_LOW_VOLTAGE) {
        mV_max = CONFIG_BATTERY_LIB_MAX_VOLTAGE;
        mV_min = CONFIG_BATTERY_LIB_LOW_VOLTAGE;
        upper = 100;
        lower = CONFIG_BATTERY_LIB_LOW_VOLTAGE_PERCENT;
    } else if (batt_mV <= CONFIG_BATTERY_LIB_MIN_OPERATING_VOLTAGE) {
        val = 0;
    } else {
        mV_max = CONFIG_BATTERY_LIB_LOW_VOLTAGE;
        mV_min = CONFIG_BATTERY_LIB_MIN_OPERATING_VOLTAGE;
        upper = CONFIG_BATTERY_LIB_LOW_VOLTAGE_PERCENT;
        lower = 0;
    }

    if (val != 0) {
        numerator = (batt_mV - mV_min) * (upper - lower) * multiplier;
        denominator = (mV_max - mV_min) * multiplier;
        val = (lower + (numerator / denominator));
    }

    LOG_DBG("Calc Battery Percentage is %d%%", val);

    if (val > 110) { // 110 allows for 10% variance in adc read
        LOG_ERR("%d is too large! Something is wrong", val);
        ret = 0;
    } else {
        ret = (uint8_t)val;
    }

    return ret;
}
