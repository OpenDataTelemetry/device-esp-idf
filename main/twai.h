#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

#include "esp_types.h"
#include "esp_event.h"
#include "esp_err.h"
#include "driver/twai.h"

#define TX_GPIO_NUM CONFIG_TWAI_TX_GPIO_NUM
#define RX_GPIO_NUM CONFIG_TWAI_RX_GPIO_NUM

#define TWAI_API_LOCK(c) xSemaphoreTakeRecursive(c->api_lock, portMAX_DELAY)
#define TWAI_API_UNLOCK(c) xSemaphoreGiveRecursive(c->api_lock)

#define TWAI_TASK_PRIORITY 5
#define TWAI_TASK_STACK (6 * 1024)

  ESP_EVENT_DECLARE_BASE(ESP_TWAI_EVENT);

#define TWAI_GENERAL_CONFIG()            \
  {                                      \
      .mode = TWAI_MODE_LISTEN_ONLY,     \
      .tx_io = TX_GPIO_NUM,              \
      .rx_io = RX_GPIO_NUM,              \
      .clkout_io = TWAI_IO_UNUSED,       \
      .bus_off_io = TWAI_IO_UNUSED,      \
      .tx_queue_len = 0,                 \
      .rx_queue_len = 5,                 \
      .alerts_enabled = TWAI_ALERT_NONE, \
      .clkout_divider = 0}

#define ESP_TWAI_CLIENT_CONFIG_DEFAULT() \
  {                                      \
      TWAI_FILTER_CONFIG_ACCEPT_ALL(),   \
      TWAI_TIMING_CONFIG_1MBITS(),       \
      TWAI_GENERAL_CONFIG()}

  typedef struct esp_twai_client *esp_twai_client_handle_t;

  typedef enum
  {
    TWAI_STATE_INIT = 0,
    TWAI_STATE_DISCONNECTED,
    TWAI_STATE_CONNECTED,
    TWAI_STATE_WAIT_RECONNECT,
  } twai_client_state_t;

  typedef enum esp_twai_error_type_t
  {
    TWAI_ERROR_TYPE_NONE = 0,
    TWAI_ERROR_TYPE_TCP_TRANSPORT,
    TWAI_ERROR_TYPE_CONNECTION_REFUSED,
    TWAI_ERROR_TYPE_SUBSCRIBE_FAILED
  } esp_twai_error_type_t;

  typedef struct twai_connection
  {
    twai_message_t outbound_message;
    uint8_t *buffer;
    size_t buffer_length;
    // twai_connect_info_t information;

  } twai_connection_t;
  typedef struct twai_state
  {
    uint8_t *in_buffer;
    int in_buffer_length;
    size_t message_length;
    size_t in_buffer_read_len;
    twai_connection_t connection;
    uint16_t pending_msg_id;
    int pending_msg_type;
    int pending_publish_qos;
  } twai_state2_t;

  typedef struct esp_twai_error_codes
  {
    /* compatible portion of the struct corresponding to struct esp_tls_last_error
     */
    // esp_err_t esp_tls_last_esp_err; /*!< last esp_err code reported from esp-tls
    //                                  component */
    // int esp_tls_stack_err; /*!< tls specific error code reported from underlying
    //                         tls stack */
    // int esp_tls_cert_verify_flags; /*!< tls flags reported from underlying tls
    //                                 stack during certificate verification */
    /* esp-mqtt specific structure extension */
    esp_twai_error_type_t
        error_type; /*!< error type referring to the source of the error */
    // esp_mqtt_connect_return_code_t
    // connect_return_code; /*!< connection refused error code reported from
    // *MQTT* broker on connection */
    /* tcp_transport extension */
    int esp_transport_sock_errno; /*!< errno from the underlying socket */

  } esp_twai_error_codes_t;

  typedef enum esp_twai_event_id_t
  {
    TWAI_EVENT_ANY = -1,
    TWAI_EVENT_ERROR =
        0,                     /*!< on error event, additional context: connection return code, error
                                handle from esp_tls (if supported) */
    TWAI_EVENT_CONNECTED,      /*!< connected event, additional context:
                                session_present flag */
    TWAI_EVENT_DISCONNECTED,   /*!< disconnected event */
    TWAI_EVENT_SUBSCRIBED,     /*!< subscribed event, additional context:
                                  - msg_id               message id
                                  - error_handle         `error_type` in case subscribing failed
                                  - data                 pointer to broker response, check for errors.
                                  - data_len             length of the data for this
                                event
                                  */
    TWAI_EVENT_UNSUBSCRIBED,   /*!< unsubscribed event, additional context:  msg_id */
    TWAI_EVENT_PUBLISHED,      /*!< published event, additional context:  msg_id */
    TWAI_EVENT_DATA,           /*!< data event, additional context:
                                  - msg_id               message id
                                  - topic                pointer to the received topic
                                  - topic_len            length of the topic
                                  - data                 pointer to the received data
                                  - data_len             length of the data for this event
                                  - current_data_offset  offset of the current data for
                                this event
                                  - total_data_len       total length of the data received
                                  - retain               retain flag of the message
                                  - qos                  QoS level of the message
                                  - dup                  dup flag of the message
                                  Note: Multiple TWAI_EVENT_DATA could be fired for one
                                message, if it is         longer than internal buffer. In that
                                case only first event contains topic         pointer and length,
                                other contain data only with current data length         and
                                current data offset updating.
                                   */
    TWAI_EVENT_BEFORE_CONNECT, /*!< The event occurs before connecting */
    TWAI_EVENT_DELETED,        /*!< Notification on delete of one message from the
                                internal outbox,        if the message couldn't have been sent
                                and acknowledged before expiring        defined in
                                OUTBOX_EXPIRED_TIMEOUT_MS.        (events are not posted upon
                                deletion of successfully acknowledged messages)
                                  - This event id is posted only if
                                TWAI_REPORT_DELETED_MESSAGES==1
                                  - Additional context: msg_id (id of the deleted
                                message).
                                  */
    TWAI_USER_EVENT,           /*!< Custom event used to queue tasks into mqtt event handler
                                All fields from the esp_mqtt_event_t type could be used to pass
                                an additional context data to the handler.
                                */
  } esp_twai_event_id_t;

  typedef struct esp_twai_event_t
  {
    esp_twai_event_id_t event_id;    /*!< *MQTT* event type */
    esp_twai_client_handle_t client; /*!< *MQTT* client handle for this event */
    char *data;                      /*!< Data associated with this event */
    int data_len;                    /*!< Length of the data for this event */
    int total_data_len;              /*!< Total length of the data (longer data are supplied
                                      with multiple events) */
    int current_data_offset;         /*!< Actual offset for the data associated with this
                                      event */
    char *topic;                     /*!< Topic associated with this event */
    int topic_len;                   /*!< Length of the topic for this event associated with this
                                      event */
    int msg_id;                      /*!< *MQTT* messaged id of message */
    int session_present;             /*!< *MQTT* session_present flag for connection event */
    esp_twai_error_codes_t
        *error_handle; /*!< esp-mqtt error handle including esp-tls errors as well
                            as internal *MQTT* errors */
    //     bool retain; /*!< Retained flag of the message associated with this event */
    //     int qos;     /*!< QoS of the messages associated with this event */
    //     bool dup;    /*!< dup flag of the message associated with this event */
    //     esp_mqtt_protocol_ver_t protocol_ver;   /*!< MQTT protocol version used for connection, defaults to value from menuconfig*/
    // #ifdef CONFIG_MQTT_PROTOCOL_5
    //     esp_mqtt5_event_property_t *property; /*!< MQTT 5 property associated with this event */
    // #endif

  } esp_twai_event_t;

  typedef esp_twai_event_t *esp_twai_event_handle_t;

  typedef struct
  {
    twai_filter_config_t f_config;
    twai_timing_config_t t_config;
    twai_general_config_t g_config;
    struct twai_task_t
    {
      int priority;   /*!< *MQTT* task priority*/
      int stack_size; /*!< *MQTT* task stack size*/
    } task;
  } esp_twai_client_config_t;

  esp_twai_client_handle_t esp_twai_client_init(const esp_twai_client_config_t *config);

  esp_err_t esp_twai_client_register_event(esp_twai_client_handle_t client, esp_twai_event_id_t event, esp_event_handler_t event_handler, void *event_handler_arg);

  //   typedef struct twai_connection {
  //     twai_message_t outbound_message;
  //     uint8_t *buffer;
  //     size_t buffer_length;
  //     // twai_connect_info_t information;

  // } twai_connection_t;

  //   typedef struct twai_state {
  //     uint8_t *in_buffer;
  //     int in_buffer_length;
  //     size_t message_length;
  //     size_t in_buffer_read_len;
  //     twai_connection_t connection;
  //     uint16_t pending_msg_id;
  //     int pending_msg_type;
  //     int pending_publish_qos;
  // } twai_state_t;
  typedef struct
  {
    esp_event_loop_handle_t event_loop_handle;
    int task_stack;
    int task_prio;
    twai_filter_config_t driver_f_config;
    twai_timing_config_t driver_t_config;
    twai_general_config_t driver_g_config;

    // char *uri;
    // char *host;
    // char *path;
    // char *scheme;
    // int port;
    // bool auto_reconnect;
    // int network_timeout_ms;
    // int refresh_connection_after_ms;
    // int reconnect_timeout_ms;
    // char **alpn_protos;
    // int num_alpn_protos;
    // char *clientkey_password;
    // int clientkey_password_len;
    // bool use_global_ca_store;
    // esp_err_t ((*crt_bundle_attach)(void *conf));
    // const char *cacert_buf;
    // size_t cacert_bytes;
    // const char *clientcert_buf;
    // size_t clientcert_bytes;
    // const char *clientkey_buf;
    // size_t clientkey_bytes;
    // const struct psk_key_hint *psk_hint_key;
    // bool skip_cert_common_name_check;
    // const char *common_name;
    // bool use_secure_element;
    // void *ds_data;
    // int message_retransmit_timeout;
    // uint64_t outbox_limit;
    // esp_transport_handle_t transport;
    // struct ifreq * if_name;
  } twai_config_storage_t;

  struct esp_twai_client
  {
    // esp_transport_list_handle_t transport_list;
    // esp_transport_handle_t transport;
    twai_config_storage_t *config;
    twai_state2_t twai_state;
    _Atomic twai_client_state_t state;
    uint64_t refresh_connection_tick;
    int64_t keepalive_tick;
    uint64_t reconnect_tick;
    //     int64_t keepalive_tick;
    //     uint64_t reconnect_tick;
    // #ifdef MQTT_PROTOCOL_5
    //     mqtt5_config_storage_t *mqtt5_config;
    //     uint16_t send_publish_packet_count; // This is for MQTT v5.0 flow control
    // #endif
    int wait_timeout_ms;
    //     int auto_reconnect;
    esp_twai_event_t event;
    bool run;
    bool wait_for_ping_resp;
    // outbox_handle_t outbox;
    EventGroupHandle_t status_bits;
    SemaphoreHandle_t api_lock;
    TaskHandle_t task_handle;
  };

  esp_err_t esp_twai_client_start(esp_twai_client_handle_t client);

  esp_err_t twai_client_deinit(esp_twai_client_handle_t twai);

  esp_err_t twai_client_remove_handler(esp_twai_client_handle_t twai, esp_event_handler_t event_handler);

#ifdef __cplusplus
}
#endif