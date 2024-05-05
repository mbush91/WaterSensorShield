/**
 *  @brief Manage MQTT communication for PointPerfect GNSS accuracy enhancement.
 * aka Thingstream.io technology.
 */

#include "pss_mqtt.h"
#include "pss_nrf_lte.h"
#if __has_include("gen/pss_mqtt_certs.h")
#include "gen/pss_mqtt_certs.h"
#else
#include "pss_mqtt_certs.h"
#endif
#include <math.h>
#include <modem/modem_key_mgmt.h>
#include <modem/nrf_modem_lib.h>
#include <net/mqtt_helper.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>

LOG_MODULE_REGISTER(pss_mqtt, CONFIG_PSS_MQTT_LOG_LEVEL);

// Local Macro Definitions /////////////////
#define MQTT_HOSTNAME CONFIG_PSS_MQTT_HOST
#define CLIENT_ID_BUF_SIZE 64
#define SUBSCRIBE_ID 1001

// Global Variable Declarations /////////////////
size_t client_id_size = CLIENT_ID_BUF_SIZE;
char client_id[CLIENT_ID_BUF_SIZE];
K_SEM_DEFINE(pss_mqtt_do_disconnect_sem, 0, 1); // Ready for subscription

// Local Variable Declarations /////////////////
static K_SEM_DEFINE(do_connection_sem, 0, 1); // Ready for connection
static K_SEM_DEFINE(on_connection_sem, 0, 1); // Ready for subscription

static bool mqtt_connected = false;
static bool mqtt_has_error = true;

const uint8_t status_topic[] = "homeassistant/sump/availability";
struct mqtt_topic lwt_topic = {
  .qos = 1,
  .topic = {
    .utf8 = status_topic,
    .size = sizeof(status_topic) - 1
  }
};
struct mqtt_utf8 lwt_msg = {
  .utf8 = "offline",
  .size = 7
};

static const struct gpio_dt_spec led2 = GPIO_DT_SPEC_GET(DT_NODELABEL(led1), gpios);

static int32_t init_led(void)
{
  int32_t ret;
  if (!gpio_is_ready_dt(&led2))
  {
    return 0;
  }

  ret = gpio_pin_configure_dt(&led2, GPIO_OUTPUT_ACTIVE);
  if (ret < 0)
  {
    return 0;
  }

  gpio_pin_set_dt(&led2,0);

  return 0;
}

bool pss_mqtt_has_error(void)
{
  return mqtt_has_error;
}

bool pss_mqtt_connected(void)
{
  return mqtt_connected;
}

int32_t pss_mqtt_provision(void)
{
  int32_t err;
  bool exists;

#if defined(PSS_MQTT_CERTS_AVAILABLE)
  // Provision the Root Certificate
  (void)modem_key_mgmt_exists(CONFIG_MY_MQTT_HELPER_SEC_TAG,
                              MODEM_KEY_MGMT_CRED_TYPE_CA_CHAIN,
                              &exists);
  if (exists && !IS_ENABLED(CONFIG_PSS_MQTT_FORCE_PROVISION))
  { // lint !e774 !e506
    LOG_INF("Cred type: %d for sec tag %d",
            MODEM_KEY_MGMT_CRED_TYPE_CA_CHAIN,
            CONFIG_MY_MQTT_HELPER_SEC_TAG);
  }
  else
  {
    err = modem_key_mgmt_write(CONFIG_MY_MQTT_HELPER_SEC_TAG,
                               MODEM_KEY_MGMT_CRED_TYPE_CA_CHAIN,
                               root_ca,
                               sizeof(root_ca) - 1); // lint !e778
    if (err)
    {
      LOG_ERR("Failed to register CA certificate: %d", err);
      return err;
    }
    else
    {
      LOG_INF("Provisioned CA Certificate");
    }
  }

  // Provision the Ublox Certificate
  (void)modem_key_mgmt_exists(CONFIG_MY_MQTT_HELPER_SEC_TAG,
                              MODEM_KEY_MGMT_CRED_TYPE_PUBLIC_CERT,
                              &exists);
  if (exists && !IS_ENABLED(CONFIG_PSS_MQTT_FORCE_PROVISION))
  { // lint !e774 !e506
    LOG_INF("Cred type: %d for sec tag %d",
            MODEM_KEY_MGMT_CRED_TYPE_PUBLIC_CERT,
            CONFIG_MY_MQTT_HELPER_SEC_TAG);
  }
  else
  {
    err = modem_key_mgmt_write(CONFIG_MY_MQTT_HELPER_SEC_TAG,
                               MODEM_KEY_MGMT_CRED_TYPE_PUBLIC_CERT,
                               ublox_ca_certificate,
                               sizeof(ublox_ca_certificate) - 1); // lint !e778
    if (err)
    {
      LOG_ERR("Failed to register private key: %d", err);
      return err;
    }
    else
    {
      LOG_INF("Provisioned Client Certificate");
    }
  }

  // Provision the Private Key
  (void)modem_key_mgmt_exists(CONFIG_MY_MQTT_HELPER_SEC_TAG,
                              MODEM_KEY_MGMT_CRED_TYPE_PRIVATE_CERT,
                              &exists);
  if (exists && !IS_ENABLED(CONFIG_PSS_MQTT_FORCE_PROVISION))
  { // lint !e774 !e506
    LOG_INF("Cred type: %d for sec tag %d",
            MODEM_KEY_MGMT_CRED_TYPE_PRIVATE_CERT,
            CONFIG_MY_MQTT_HELPER_SEC_TAG);
  }
  else
  {
    err = modem_key_mgmt_write(CONFIG_MY_MQTT_HELPER_SEC_TAG,
                               MODEM_KEY_MGMT_CRED_TYPE_PRIVATE_CERT,
                               ublox_key,
                               sizeof(ublox_key) - 1); // lint !e778

    if (err)
    {
      LOG_ERR("Failed to register private key: %d", err);
      return err;
    }
    else
    {
      LOG_INF("Provisioned Private Key Certificate");
    }
  }

  // Provision the Client ID
  (void)modem_key_mgmt_exists(CONFIG_MY_MQTT_HELPER_SEC_TAG,
                              MODEM_KEY_MGMT_CRED_TYPE_IDENTITY,
                              &exists);
  if (exists && !IS_ENABLED(CONFIG_PSS_MQTT_FORCE_PROVISION))
  { // lint !e774 !e506
    LOG_INF("Cred type: %d for sec tag %d",
            MODEM_KEY_MGMT_CRED_TYPE_IDENTITY,
            CONFIG_MY_MQTT_HELPER_SEC_TAG);
  }
  else
  {
    err = modem_key_mgmt_write(CONFIG_MY_MQTT_HELPER_SEC_TAG,
                               MODEM_KEY_MGMT_CRED_TYPE_IDENTITY,
                               new_client_id,
                               sizeof(new_client_id) - 1); // lint !e778

    if (err)
    {
      LOG_ERR("Failed to register client id: %d", err);
      return err;
    }
    else
    {
      LOG_INF("Provisioned Client ID");
    }
  }
#else
  ARG_UNUSED(exists);
  ARG_UNUSED(new_client_id);
  ARG_UNUSED(ublox_ca_certificate);
  ARG_UNUSED(ublox_key);
  ARG_UNUSED(root_ca);
#endif /* defined(PSS_MQTT_CERTS_AVAILABLE) */

  err = modem_key_mgmt_read(CONFIG_MY_MQTT_HELPER_SEC_TAG,
                            MODEM_KEY_MGMT_CRED_TYPE_IDENTITY,
                            client_id,
                            &client_id_size);
  LOG_INF("Client ID: %s", client_id);

  return err;
};

/**
 * @brief Callback from a connection event.
 *
 * @param return_code The return codes of the connection attempt
 */
static void mqtt_connected_cb(enum mqtt_conn_return_code return_code)
{
  if (return_code == MQTT_CONNECTION_ACCEPTED)
  {
    LOG_INF("MQTT Connected successfully");
    mqtt_connected = true;
    gpio_pin_set_dt(&led2,1);
    mqtt_has_error = false;
    // Allow subscriptions to start
    k_sem_give(&on_connection_sem);
  }
  else
  {
    LOG_ERR("MQTT failed to connect. Return code is %d", (int)return_code);
    mqtt_connected = false;
    mqtt_has_error = true;
    gpio_pin_set_dt(&led2,0);
  }
}

/**
 * @brief Callback from a disconnection event
 *
 * @param result 0 if successful disconnect, negative otherwise
 */
static void mqtt_disconnected_cb(int result)
{
  LOG_WRN("MQTT Disconnected %d", (int)result);
  mqtt_connected = false;
  gpio_pin_set_dt(&led2,0);
  k_sem_give(&do_connection_sem);
}

/**
 * @brief Callback from an error in the publish event
 *
 * @param error The only documented error is when the payload is larger than the buffer
 */
static void mqtt_error_cb(enum mqtt_helper_error error)
{
  LOG_ERR("The received payload is larger than the payload buffer, error %d", error);
  mqtt_has_error = true;
}

/**
 * @brief Callback from a publish event
 *
 * @param topic The topic that has published by PointPerfect
 * @param payload The contents of that published topic
 */
static void mqtt_publish_cb(struct mqtt_helper_buf topic, struct mqtt_helper_buf payload)
{
  int32_t err = 0;

  LOG_INF("Topic %s - Payload size: %d", topic.ptr, payload.size);

  // TODO
  // err = pss_mqtt_ubx_process_payload(topic, payload);
  if (err < 0)
  {
    LOG_ERR("Error processing payload, err %d", err);
    mqtt_has_error = true;
  }
  else
  {
    mqtt_has_error = false;
  }

#if IS_ENABLED(CONFIG_PSS_MQTT_DUMP_HEX)
  LOG_INF("From Topic: %s", topic.ptr);
  LOG_HEXDUMP_INF(payload.ptr, payload.size, "Payload Data: ");
#endif
}

/**
 * @brief Callback from a subscribe event
 *
 * @param message_id The message ID (should match the one we provided)
 * @param result
 */
static void mqtt_subscribe_cb(uint16_t message_id, int result)
{
  if ((message_id == SUBSCRIBE_ID) && (result == (int)MQTT_SUBACK_SUCCESS_QoS_0))
  {
    LOG_INF("Subscribed to topics, with ID %d", SUBSCRIBE_ID);
    mqtt_has_error = false;
  }
  else if (result)
  {
    LOG_ERR("Topic subscription failed, error: %d", result);
    mqtt_has_error = true;
  }
  else
  {
    LOG_WRN("Subscribed to unknown topic, id: %d", message_id);
  }
}

/**
 * @brief Initiate a connection to the MQTT host
 * Results are available in the connection callback function
 * Failure to connect is displayed in LOG_ERR message
 */
static void pss_mqtt_connect(void)
{
  int32_t err;

  struct mqtt_helper_conn_params conn_params = {0};

  conn_params.hostname.ptr = MQTT_HOSTNAME;
  conn_params.hostname.size = strlen(MQTT_HOSTNAME);
  conn_params.device_id.ptr = client_id;
  conn_params.device_id.size = client_id_size;
  conn_params.will_topic = &lwt_topic;
  conn_params.will_message = &lwt_msg;
  conn_params.will_retain = 1;

  err = mqtt_helper_connect(&conn_params);
  if (err)
  {
    LOG_ERR("MQTT Helper connected failed, err: %d", err);
    mqtt_has_error = true;
    k_sem_give(&do_connection_sem);
    return;
  }
}

/**
 * @brief Initiate a subscription to a hardcoded set of topics
 * QoS level is set to: MQTT_QOS_0_AT_MOST_ONCE
 *
 * @retval 0 The subscription attempt was successful
 */
static int pss_mqtt_subscribe(void)
{
  int err;

  struct mqtt_subscription_list list; // TODO = pss_mqtt_ubx_get_sub_list(SUBSCRIBE_ID);

  for (size_t i = 0; i < list.list_count; i++)
  {
    LOG_INF("Attempting subscription to: %s", (char *)list.list[i].topic.utf8);
  }

  err = mqtt_helper_subscribe(&list);
  if (err)
  {
    LOG_ERR("Failed to subscribe to topics, error: %d", err);
    mqtt_has_error = true;
    return -1;
  }
  mqtt_has_error = false;
  return 0;
}

int32_t pss_mqtt_publish(const uint8_t *pub_topic, char *msg, uint8_t QOS)
{
  int32_t err = 0;

  struct mqtt_publish_param param = {
      .message.payload.data = msg,
      .message.payload.len = strlen(msg),
      .message.topic.qos = QOS,
      .message_id = k_uptime_get_32(),
      .message.topic.topic.utf8 = pub_topic,
      .message.topic.topic.size = strlen(pub_topic),
      .retain_flag = 1
  };

  if (mqtt_connected)
  {
    err = mqtt_helper_publish(&param);
    if (err)
    {
      LOG_ERR("Failed to send payload, err: %d", err);
      return err;
    }

    LOG_INF("Published message: \"%.*s\" on topic: \"%.*s\"", param.message.payload.len,
            param.message.payload.data,
            param.message.topic.topic.size,
            param.message.topic.topic.utf8);
  }
  else
  {
    LOG_WRN("Cannot publish to %s, no mqtt connection", pub_topic);
  }

  return err;
}

int32_t pss_mqtt_init(void)
{
  int32_t err;
  struct mqtt_helper_cfg init_cfg = {0};


  /* Config MQTT Helper Callbacks */
  init_cfg.cb.on_connack = mqtt_connected_cb;
  init_cfg.cb.on_disconnect = mqtt_disconnected_cb;
  init_cfg.cb.on_error = mqtt_error_cb;
  init_cfg.cb.on_publish = mqtt_publish_cb;
  init_cfg.cb.on_suback = mqtt_subscribe_cb;

  err = mqtt_helper_init(&init_cfg);
  if (err)
  {
    LOG_ERR("Could not init mqtt helper, err: %d", err);
    return -1;
  }

#if !IS_ENABLED(CONFIG_PSS_MQTT_DISABLE_SUBSCRIPTIONS)
  k_sem_give(&do_connection_sem);
#endif

  init_led();

  return err;
}

void pss_mqtt_main(void)
{
  if (pss_nrf_lte_connected())
  {
    if (0 == k_sem_take(&pss_mqtt_do_disconnect_sem, K_NO_WAIT))
    {
      (void)mqtt_helper_disconnect();
    }
    else if (0 == k_sem_take(&do_connection_sem, K_NO_WAIT))
    {
      pss_mqtt_connect();
    }
    else if (0 == k_sem_take(&on_connection_sem, K_NO_WAIT))
    {
      // void pss_mqtt_startup(void);
    }
  }
}
