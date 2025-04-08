/*UART ASYNC*/
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "driver/uart.h"
#include "string.h"
#include "driver/gpio.h"
#include <stdio.h>
#include <stdlib.h>

/*NMEA*/
#include <stdio.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "nmea_parser.h"

static const char *TAG_GPS = "gps_demo";

#define TIME_ZONE (0)    // Beijing Time
#define YEAR_BASE (2000) // date in GPS starts from 2000

/*CAN*/
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "esp_err.h"
#include "driver/twai.h"

/*UART ASYNC*/
static const int RX_BUF_SIZE = 2048;
#define TXD_PIN (GPIO_NUM_18)
#define RXD_PIN (GPIO_NUM_17)


/*MQTT*/
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "protocol_examples_common.h"
#include "mqtt_client.h"


/* --------------------- Definitions and static variables ------------------ */
/*CAN*/
#define NO_OF_ITERS 3
#define RX_TASK_PRIO 9
#define TX_GPIO_NUM CONFIG_EXAMPLE_TX_GPIO_NUM
#define RX_GPIO_NUM CONFIG_EXAMPLE_RX_GPIO_NUM
#define TAG_TWAI "TWAI Listen Only"

#define ID_MASTER_STOP_CMD 0x0A0
#define ID_MASTER_START_CMD 0x0A1
#define ID_MASTER_PING 0x0A2
#define ID_SLAVE_STOP_RESP 0x0B0
#define ID_SLAVE_DATA 0x0B1
#define ID_SLAVE_PING_RESP 0x0B2



static const twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();
static const twai_timing_config_t t_config = TWAI_TIMING_CONFIG_1MBITS();
// Set TX queue length to 0 due to listen only mode
static const twai_general_config_t g_config = {.mode = TWAI_MODE_LISTEN_ONLY,
                                               .tx_io = TX_GPIO_NUM,
                                               .rx_io = RX_GPIO_NUM,
                                               .clkout_io = TWAI_IO_UNUSED,
                                               .bus_off_io = TWAI_IO_UNUSED,
                                               .tx_queue_len = 0,
                                               .rx_queue_len = 5,
                                               .alerts_enabled = TWAI_ALERT_NONE,
                                               .clkout_divider = 0};
static SemaphoreHandle_t rx_sem;

/*MQTT*/
static const char *TAG_MQTT = "mqtt5_example";

static void log_error_if_nonzero(const char *message, int error_code)
{
  if (error_code != 0)
  {
    ESP_LOGE(TAG_MQTT, "Last error %s: 0x%x", message, error_code);
  }
}

static esp_mqtt5_user_property_item_t user_property_arr[] = {
    {"board", "esp32"},
    {"u", "user"},
    {"p", "password"}};

#define USE_PROPERTY_ARR_SIZE sizeof(user_property_arr) / sizeof(esp_mqtt5_user_property_item_t)

static esp_mqtt5_publish_property_config_t publish_property = {
    .payload_format_indicator = 1,
    .message_expiry_interval = 1000,
    .topic_alias = 0,
    .response_topic = "/topic/test/response",
    .correlation_data = "123456",
    .correlation_data_len = 6,
};

static esp_mqtt5_subscribe_property_config_t subscribe_property = {
    .subscribe_id = 25555,
    .no_local_flag = false,
    .retain_as_published_flag = false,
    .retain_handle = 0,
    .is_share_subscribe = true,
    .share_name = "group1",
};

static esp_mqtt5_subscribe_property_config_t subscribe1_property = {
    .subscribe_id = 25555,
    .no_local_flag = true,
    .retain_as_published_flag = false,
    .retain_handle = 0,
};

static esp_mqtt5_unsubscribe_property_config_t unsubscribe_property = {
    .is_share_subscribe = true,
    .share_name = "group1",
};

static esp_mqtt5_disconnect_property_config_t disconnect_property = {
    .session_expiry_interval = 60,
    .disconnect_reason = 0,
};

static void print_user_property(mqtt5_user_property_handle_t user_property)
{
  if (user_property)
  {
    uint8_t count = esp_mqtt5_client_get_user_property_count(user_property);
    if (count)
    {
      esp_mqtt5_user_property_item_t *item = malloc(count * sizeof(esp_mqtt5_user_property_item_t));
      if (esp_mqtt5_client_get_user_property(user_property, item, &count) == ESP_OK)
      {
        for (int i = 0; i < count; i++)
        {
          esp_mqtt5_user_property_item_t *t = &item[i];
          ESP_LOGI(TAG_MQTT, "key is %s, value is %s", t->key, t->value);
          free((char *)t->key);
          free((char *)t->value);
        }
      }
      free(item);
    }
  }
}

/* --------------------------- Tasks and Functions -------------------------- */

static void twai_receive_task(void *arg)
{
  xSemaphoreTake(rx_sem, portMAX_DELAY);

  while (1)
  {
    twai_message_t rx_msg;
    twai_receive(&rx_msg, portMAX_DELAY);
    uint32_t data = 0;
    for (int i = 0; i < rx_msg.data_length_code; i++)
    {
      data |= (rx_msg.data[i] << (i * 8));
    }
    ESP_LOGI(TAG_TWAI, "Received %u data bytes with value %" PRIu32, rx_msg.data_length_code, data);
    vTaskDelay(100);
  }

  xSemaphoreGive(rx_sem);
  vTaskDelete(NULL);
}

int sendData(const char *logName, const char *data)
{
  const int len = strlen(data);
  const int txBytes = uart_write_bytes(UART_NUM_1, data, len);
  ESP_LOGI(logName, "Wrote %d bytes", txBytes);
  return txBytes;
}

void init(void)
{
  const uart_config_t uart_config = {
      .baud_rate = 115200,
      .data_bits = UART_DATA_8_BITS,
      .parity = UART_PARITY_DISABLE,
      .stop_bits = UART_STOP_BITS_1,
      .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
      .source_clk = UART_SCLK_DEFAULT,
  };
  // We won't use a buffer for sending data.
  uart_driver_install(UART_NUM_1, RX_BUF_SIZE * 2, 0, 0, NULL, 0);
  uart_param_config(UART_NUM_1, &uart_config);
  uart_set_pin(UART_NUM_1, TXD_PIN, RXD_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
}

void send_gps_task(void)
{
  static const char *GPS_TASK_TAG = "GPS_TASK";
  sendData(GPS_TASK_TAG, "AT\r\n");
  vTaskDelay(1000 / portTICK_PERIOD_MS);
  sendData(GPS_TASK_TAG, "AT\r\n");
  vTaskDelay(1000 / portTICK_PERIOD_MS);
  sendData(GPS_TASK_TAG, "AT\r\n");
  vTaskDelay(1000 / portTICK_PERIOD_MS);

  sendData(GPS_TASK_TAG, "AT+CGNSSPWR=1\r\n\r\n");
  vTaskDelay(1000 / portTICK_PERIOD_MS);
  sendData(GPS_TASK_TAG, "AT+CGPSHOT\r\n");
  vTaskDelay(1000 / portTICK_PERIOD_MS);
  sendData(GPS_TASK_TAG, "AT+CGNSSTST=1\r\n");
  vTaskDelay(10000 / portTICK_PERIOD_MS);

  sendData(GPS_TASK_TAG, "AT+CGPSINFO=1\r\n");
  vTaskDelay(1000 / portTICK_PERIOD_MS);
  sendData(GPS_TASK_TAG, "AT+CGNSSINFO=1\r\n");
  vTaskDelay(1000 / portTICK_PERIOD_MS);
  sendData(GPS_TASK_TAG, "AT+CGNSSPORTSWITCH=0,1\r\n");
  vTaskDelay(1000 / portTICK_PERIOD_MS);
  sendData(GPS_TASK_TAG, "AT+CGNSSNMEA=1,1,1,1,1,1,0,0,0,0\\r\n");
  vTaskDelay(1000 / portTICK_PERIOD_MS);
}

// static void uart_tx_async_task(void *arg)
// {
//   static const char *TX_TASK_TAG = "TX_TASK";
//   esp_log_level_set(TX_TASK_TAG, ESP_LOG_INFO);
//   while (1)
//   {
//     sendData(TX_TASK_TAG, "AT\r\n");
//     vTaskDelay(1000 / portTICK_PERIOD_MS);

//   }
// }

static void uart_rx_async_task(void *arg)
{
  static const char *RX_TASK_TAG = "RX_TASK";
  esp_log_level_set(RX_TASK_TAG, ESP_LOG_INFO);
  uint8_t *data = (uint8_t *)malloc(RX_BUF_SIZE + 1);
  while (1)
  {
    const int rxBytes = uart_read_bytes(UART_NUM_1, data, RX_BUF_SIZE, 1000 / portTICK_PERIOD_MS);
    if (rxBytes > 0)
    {
      data[rxBytes] = 0;
      ESP_LOGI(RX_TASK_TAG, "Read %d bytes: '%s'", rxBytes, data);
      ESP_LOG_BUFFER_HEXDUMP(RX_TASK_TAG, data, rxBytes, ESP_LOG_INFO);
    }
  }
  free(data);
}

static void gps_event_handler(void *event_handler_arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
  gps_t *gps = NULL;
  switch (event_id)
  {
  case GPS_UPDATE:
    gps = (gps_t *)event_data;
    /* print information parsed from GPS statements */
    ESP_LOGI(TAG_GPS, "%d/%d/%d %d:%d:%d => \r\n"
                  "\t\t\t\t\t\tlatitude   = %.05f°N\r\n"
                  "\t\t\t\t\t\tlongitude = %.05f°E\r\n"
                  "\t\t\t\t\t\taltitude   = %.02fm\r\n"
                  "\t\t\t\t\t\tspeed      = %fm/s",
             gps->date.year + YEAR_BASE, gps->date.month, gps->date.day,
             gps->tim.hour + TIME_ZONE, gps->tim.minute, gps->tim.second,
             gps->latitude, gps->longitude, gps->altitude, gps->speed);
    break;
  case GPS_UNKNOWN:
    /* print unknown statements */
    ESP_LOGW(TAG_GPS, "Unknown statement:%s", (char *)event_data);
    break;
  default:
    break;
  }
}

static void mqtt5_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
  ESP_LOGD(TAG_MQTT, "Event dispatched from event loop base=%s, event_id=%" PRIi32, base, event_id);
  esp_mqtt_event_handle_t event = event_data;
  esp_mqtt_client_handle_t client = event->client;
  int msg_id;

  ESP_LOGD(TAG_MQTT, "free heap size is %" PRIu32 ", minimum %" PRIu32, esp_get_free_heap_size(), esp_get_minimum_free_heap_size());
  switch ((esp_mqtt_event_id_t)event_id)
  {
  case MQTT_EVENT_CONNECTED:
    ESP_LOGI(TAG_MQTT, "MQTT_EVENT_CONNECTED");
    print_user_property(event->property->user_property);
    esp_mqtt5_client_set_user_property(&publish_property.user_property, user_property_arr, USE_PROPERTY_ARR_SIZE);
    esp_mqtt5_client_set_publish_property(client, &publish_property);
    msg_id = esp_mqtt_client_publish(client, "/topic/qos1", "data_3", 0, 1, 1);
    esp_mqtt5_client_delete_user_property(publish_property.user_property);
    publish_property.user_property = NULL;
    ESP_LOGI(TAG_MQTT, "sent publish successful, msg_id=%d", msg_id);

    esp_mqtt5_client_set_user_property(&subscribe_property.user_property, user_property_arr, USE_PROPERTY_ARR_SIZE);
    esp_mqtt5_client_set_subscribe_property(client, &subscribe_property);
    msg_id = esp_mqtt_client_subscribe(client, "/topic/qos0", 0);
    esp_mqtt5_client_delete_user_property(subscribe_property.user_property);
    subscribe_property.user_property = NULL;
    ESP_LOGI(TAG_MQTT, "sent subscribe successful, msg_id=%d", msg_id);

    esp_mqtt5_client_set_user_property(&subscribe1_property.user_property, user_property_arr, USE_PROPERTY_ARR_SIZE);
    esp_mqtt5_client_set_subscribe_property(client, &subscribe1_property);
    msg_id = esp_mqtt_client_subscribe(client, "/topic/qos1", 2);
    esp_mqtt5_client_delete_user_property(subscribe1_property.user_property);
    subscribe1_property.user_property = NULL;
    ESP_LOGI(TAG_MQTT, "sent subscribe successful, msg_id=%d", msg_id);

    esp_mqtt5_client_set_user_property(&unsubscribe_property.user_property, user_property_arr, USE_PROPERTY_ARR_SIZE);
    esp_mqtt5_client_set_unsubscribe_property(client, &unsubscribe_property);
    msg_id = esp_mqtt_client_unsubscribe(client, "/topic/qos0");
    ESP_LOGI(TAG_MQTT, "sent unsubscribe successful, msg_id=%d", msg_id);
    esp_mqtt5_client_delete_user_property(unsubscribe_property.user_property);
    unsubscribe_property.user_property = NULL;
    break;
  case MQTT_EVENT_DISCONNECTED:
    ESP_LOGI(TAG_MQTT, "MQTT_EVENT_DISCONNECTED");
    print_user_property(event->property->user_property);
    break;
  case MQTT_EVENT_SUBSCRIBED:
    ESP_LOGI(TAG_MQTT, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
    print_user_property(event->property->user_property);
    esp_mqtt5_client_set_publish_property(client, &publish_property);
    msg_id = esp_mqtt_client_publish(client, "/topic/qos0", "data", 0, 0, 0);
    ESP_LOGI(TAG_MQTT, "sent publish successful, msg_id=%d", msg_id);
    break;
  case MQTT_EVENT_UNSUBSCRIBED:
    ESP_LOGI(TAG_MQTT, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
    print_user_property(event->property->user_property);
    esp_mqtt5_client_set_user_property(&disconnect_property.user_property, user_property_arr, USE_PROPERTY_ARR_SIZE);
    esp_mqtt5_client_set_disconnect_property(client, &disconnect_property);
    esp_mqtt5_client_delete_user_property(disconnect_property.user_property);
    disconnect_property.user_property = NULL;
    esp_mqtt_client_disconnect(client);
    break;
  case MQTT_EVENT_PUBLISHED:
    ESP_LOGI(TAG_MQTT, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
    print_user_property(event->property->user_property);
    break;
  case MQTT_EVENT_DATA:
    ESP_LOGI(TAG_MQTT, "MQTT_EVENT_DATA");
    print_user_property(event->property->user_property);
    ESP_LOGI(TAG_MQTT, "payload_format_indicator is %d", event->property->payload_format_indicator);
    ESP_LOGI(TAG_MQTT, "response_topic is %.*s", event->property->response_topic_len, event->property->response_topic);
    ESP_LOGI(TAG_MQTT, "correlation_data is %.*s", event->property->correlation_data_len, event->property->correlation_data);
    ESP_LOGI(TAG_MQTT, "content_type is %.*s", event->property->content_type_len, event->property->content_type);
    ESP_LOGI(TAG_MQTT, "TOPIC=%.*s", event->topic_len, event->topic);
    ESP_LOGI(TAG_MQTT, "DATA=%.*s", event->data_len, event->data);
    break;
  case MQTT_EVENT_ERROR:
    ESP_LOGI(TAG_MQTT, "MQTT_EVENT_ERROR");
    print_user_property(event->property->user_property);
    ESP_LOGI(TAG_MQTT, "MQTT5 return code is %d", event->error_handle->connect_return_code);
    if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT)
    {
      log_error_if_nonzero("reported from esp-tls", event->error_handle->esp_tls_last_esp_err);
      log_error_if_nonzero("reported from tls stack", event->error_handle->esp_tls_stack_err);
      log_error_if_nonzero("captured as transport's socket errno", event->error_handle->esp_transport_sock_errno);
      ESP_LOGI(TAG_MQTT, "Last errno string (%s)", strerror(event->error_handle->esp_transport_sock_errno));
    }
    break;
  default:
    ESP_LOGI(TAG_MQTT, "Other event id:%d", event->event_id);
    break;
  }
}

static void mqtt5_app_start(void)
{
  esp_mqtt5_connection_property_config_t connect_property = {
      .session_expiry_interval = 10,
      .maximum_packet_size = 1024,
      .receive_maximum = 65535,
      .topic_alias_maximum = 2,
      .request_resp_info = true,
      .request_problem_info = true,
      .will_delay_interval = 10,
      .payload_format_indicator = true,
      .message_expiry_interval = 10,
      .response_topic = "/test/response",
      .correlation_data = "123456",
      .correlation_data_len = 6,
  };

  esp_mqtt_client_config_t mqtt5_cfg = {
      .broker.address.uri = CONFIG_BROKER_URL,
      .session.protocol_ver = MQTT_PROTOCOL_V_5,
      .network.disable_auto_reconnect = true,
      .credentials.username = "123",
      .credentials.authentication.password = "456",
      .session.last_will.topic = "/topic/will",
      .session.last_will.msg = "i will leave",
      .session.last_will.msg_len = 12,
      .session.last_will.qos = 1,
      .session.last_will.retain = true,
  };

  // #if CONFIG_BROKER_URL_FROM_STDIN
  //     char line[128];

  //     if (strcmp(mqtt5_cfg.uri, "FROM_STDIN") == 0) {
  //         int count = 0;
  //         printf("Please enter url of mqtt broker\n");
  //         while (count < 128) {
  //             int c = fgetc(stdin);
  //             if (c == '\n') {
  //                 line[count] = '\0';
  //                 break;
  //             } else if (c > 0 && c < 127) {
  //                 line[count] = c;
  //                 ++count;
  //             }
  //             vTaskDelay(10 / portTICK_PERIOD_MS);
  //         }
  //         mqtt5_cfg.broker.address.uri = line;
  //         printf("Broker url: %s\n", line);
  //     } else {
  //         ESP_LOGE(TAG, "Configuration mismatch: wrong broker url");
  //         abort();
  //     }
  // #endif /* CONFIG_BROKER_URL_FROM_STDIN */

  esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt5_cfg);

  /* Set connection properties and user properties */
  esp_mqtt5_client_set_user_property(&connect_property.user_property, user_property_arr, USE_PROPERTY_ARR_SIZE);
  esp_mqtt5_client_set_user_property(&connect_property.will_user_property, user_property_arr, USE_PROPERTY_ARR_SIZE);
  esp_mqtt5_client_set_connect_property(client, &connect_property);

  /* If you call esp_mqtt5_client_set_user_property to set user properties, DO NOT forget to delete them.
   * esp_mqtt5_client_set_connect_property will malloc buffer to store the user_property and you can delete it after
   */
  esp_mqtt5_client_delete_user_property(connect_property.user_property);
  esp_mqtt5_client_delete_user_property(connect_property.will_user_property);

  /* The last argument may be used to pass data to the event handler, in this example mqtt_event_handler */
  esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt5_event_handler, NULL);
  esp_mqtt_client_start(client);
}

void app_main(void)
{
  // const char *var_name = "IDF_TARGET_ESP32S3";

  //   // Use getenv to get the value of the environment variable
  //   const char *value = getenv(var_name);

  //   // Check if the environment variable was found
  //   if (value != NULL) {
  //       printf("The value of %s is: %s\n", var_name, value);
  //   } else {
  //       printf("The environment variable %s is not set.\n", var_name);
  //   }

  // init();
  // xTaskCreate(uart_rx_async_task, "uart_rx_task", 2048 * 2, NULL, configMAX_PRIORITIES - 1, NULL);
  // xTaskCreate(tx_task, "uart_tx_async_task", 2048 * 2, NULL, configMAX_PRIORITIES - 2, NULL);

  // /*CAN*/
  rx_sem = xSemaphoreCreateBinary();
  xTaskCreatePinnedToCore(twai_receive_task, "TWAI_rx", 4096, NULL, RX_TASK_PRIO, NULL, tskNO_AFFINITY);

  // Install and start TWAI driver
  ESP_ERROR_CHECK(twai_driver_install(&g_config, &t_config, &f_config));
  ESP_LOGI(TAG_TWAI, "Driver installed");
  ESP_ERROR_CHECK(twai_start());
  ESP_LOGI(TAG_TWAI, "Driver started");

  xSemaphoreGive(rx_sem); // Start RX task
  vTaskDelay(pdMS_TO_TICKS(100));
  xSemaphoreTake(rx_sem, portMAX_DELAY); // Wait for RX task to complete

  // Stop and uninstall TWAI driver
  ESP_ERROR_CHECK(twai_stop());
  ESP_LOGI(TAG_TWAI, "Driver stopped");
  ESP_ERROR_CHECK(twai_driver_uninstall());
  ESP_LOGI(TAG_TWAI, "Driver uninstalled");

  

  

  // NMEA
  /* NMEA parser configuration */
  nmea_parser_config_t config = NMEA_PARSER_CONFIG_DEFAULT();
  /* init NMEA parser library */
  nmea_parser_handle_t nmea_hdl = nmea_parser_init(&config);
  // GPS INIT
  send_gps_task();
  /* register event handler for NMEA parser library */
  nmea_parser_add_handler(nmea_hdl, gps_event_handler, NULL);

  vTaskDelay(10000 / portTICK_PERIOD_MS);

  

  // MQTT
  ESP_LOGI(TAG_MQTT, "[APP] Startup..");
  ESP_LOGI(TAG_MQTT, "[APP] Free memory: %" PRIu32 " bytes", esp_get_free_heap_size());
  ESP_LOGI(TAG_MQTT, "[APP] IDF version: %s", esp_get_idf_version());

  esp_log_level_set("*", ESP_LOG_INFO);
  esp_log_level_set("mqtt_client", ESP_LOG_VERBOSE);
  esp_log_level_set("mqtt_example", ESP_LOG_VERBOSE);
  esp_log_level_set("transport_base", ESP_LOG_VERBOSE);
  esp_log_level_set("esp-tls", ESP_LOG_VERBOSE);
  esp_log_level_set("transport", ESP_LOG_VERBOSE);
  esp_log_level_set("outbox", ESP_LOG_VERBOSE);

  ESP_ERROR_CHECK(nvs_flash_init());
  ESP_ERROR_CHECK(esp_netif_init());
  ESP_ERROR_CHECK(esp_event_loop_create_default());

  /* This helper function configures Wi-Fi or Ethernet, as selected in menuconfig.
   * Read "Establishing Wi-Fi or Ethernet Connection" section in
   * examples/protocols/README.md for more information about this function.
   */
  ESP_ERROR_CHECK(example_connect());

  mqtt5_app_start();

// xTaskCreatePinnedToCore(twai_receive_task, "TWAI_rx", 4096, NULL, RX_TASK_PRIO, NULL, tskNO_AFFINITY);
  // Cleanup
  vSemaphoreDelete(rx_sem);
  /* unregister event handler */
  nmea_parser_remove_handler(nmea_hdl, gps_event_handler);
  /* deinit NMEA parser library */
  nmea_parser_deinit(nmea_hdl);
}
