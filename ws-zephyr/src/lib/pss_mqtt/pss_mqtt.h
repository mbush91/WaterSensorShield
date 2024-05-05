/**
 * @brief PSS MQTT, Initializes MQTT client to talk to thingstream.io location services.
 */

#ifndef PSS_MQTT_H
#define PSS_MQTT_H

#include <net/mqtt_helper.h>
#include <stdint.h>

/**
 * @brief Initialize the MQTT client and callbacks
 */
int32_t pss_mqtt_init(void);

/*
 * @brief Main thread function for the MQTT client
 */
void pss_mqtt_main(void);

/**
 * @brief Provision certificates for MQTT client.
 * This must be called after modem is initialized and offline.
 * If a cert is available but does not already exist in the modem,
 * it will be saved.
 * CONFIG_PSS_MQTT_FORCE_PROVISION forces existing certs to be
 * replaced with new certs, if available.
 *
 * @return 0 All required certificates successfully provisioned
*/
int32_t pss_mqtt_provision(void);

/**
 * @brief Returns state of MQTT connection
 *
 * @retval true MQTT is connected
 * @retval false MQTT is not connected
 */
bool pss_mqtt_connected(void);


/**
 * @brief Returns the MQTT error state
 * This is independent of whither MQTT is connected
 * or disconnected
 *
 * @retval false MQTT is functioning without errors
 * @retval true MQTT encountered errors
 */
bool pss_mqtt_has_error(void);


int32_t pss_mqtt_publish(const uint8_t* pub_topic, char * msg, uint8_t QOS);

#endif /* PSS_MQTT_H */
