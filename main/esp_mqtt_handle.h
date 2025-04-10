/**
 * @brief Default configuration for Esp MQTT Client
 *
 */
#define ESP_MQTT_CLIENT_CONFIG_DEFAULT()               \
  {                                                    \
      .broker.address.uri = CONFIG_MQTT_BROKER_URL,    \
      .session.protocol_ver = MQTT_PROTOCOL_V_5,       \
      .credentials.username = "public",                \
      .credentials.authentication.password = "public", \
  }

/**
 * @brief Default configuration for Esp MQTT Connect Property
 *
 */
#define ESP_MQTT5_CONNECTION_PROPERTY_CONFIG() \
  {                                            \
      .session_expiry_interval = 10,           \
      .maximum_packet_size = 1024,             \
      .receive_maximum = 65535,                \
      .topic_alias_maximum = 2,                \
      .request_resp_info = true,               \
      .request_problem_info = true,            \
      .will_delay_interval = 10,               \
      .payload_format_indicator = true,        \
      .message_expiry_interval = 10,           \
      .response_topic = "/test/response",      \
      .correlation_data = "123456",            \
      .correlation_data_len = 6,               \
  }

/**
 * @brief Default configuration for ESP_MQTT5_USER_PROPERTY_ITEM
 *
 */
#define ESP_MQTT5_USER_PROPERTY_ITEM() \
  {                                    \
      {"board", "esp32"},              \
      {"u", "user"},                   \
      {"p", "password"}}

/**
 * @brief Default configuration for USE_PROPERTY_ARR_SIZE
 *
 */
#define USE_PROPERTY_ARR_SIZE sizeof(user_property_arr) / sizeof(esp_mqtt5_user_property_item_t)

/**
 * @brief Default configuration for ESP_MQTT5_PUBLISH_PROPERTY_CONFIG
 *
 */
#define ESP_MQTT5_PUBLISH_PROPERTY_CONFIG()     \
  {                                             \
      .payload_format_indicator = 1,            \
      .message_expiry_interval = 1000,          \
      .topic_alias = 0,                         \
      .response_topic = "/topic/test/response", \
      .correlation_data = "123456",             \
      .correlation_data_len = 6,                \
  }

/**
 * @brief Default configuration for ESP_MQTT5_SUBSCRIBE_PROPERTY_CONFIG
 *
 */
#define ESP_MQTT5_SUBSCRIBE_PROPERTY_CONFIG() \
  {                                           \
      .subscribe_id = 25555,                  \
      .no_local_flag = false,                 \
      .retain_as_published_flag = false,      \
      .retain_handle = 0,                     \
      .is_share_subscribe = true,             \
      .share_name = "group1",                 \
  }

/**
 * @brief Default configuration for ESP_MQTT5_SUBSCRIBE_PROPERTY_CONFIG
 *
 */
#define ESP_MQTT5_SUBSCRIBE1_PROPERTY_CONFIG() \
  {                                            \
      .subscribe_id = 25555,                   \
      .no_local_flag = true,                   \
      .retain_as_published_flag = false,       \
      .retain_handle = 0,                      \
  }

/**
 * @brief Default configuration for ESP_MQTT5_UNSUBSCRIBE_PROPERTY_CONFIG
 *
 */
#define ESP_MQTT5_UNSUBSCRIBE_PROPERTY_CONFIG() \
  {                                             \
      .is_share_subscribe = true,               \
      .share_name = "group1",                   \
  }

/**
 * @brief Default configuration for ESP_MQTT5_DISCONNECT_PROPERTY_CONFIG
 *
 */
#define ESP_MQTT5_DISCONNECT_PROPERTY_CONFIG() \
  {                                            \
      .session_expiry_interval = 60,           \
      .disconnect_reason = 0,                  \
  }