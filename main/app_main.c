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
#include "nmea_parser.h"

static const char *TAG_APP = "app_main";
static const char *TAG_GPS = "app_main_gps";
static const char *TAG_MQTT = "app_main_mqtt5";

#define TIME_ZONE (0)    // Beijing Time
#define YEAR_BASE (2000) // date in GPS starts from 2000

/*CAN*/
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "esp_err.h"
#include "driver/twai.h"
#include "twai.h"

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
#include "esp_mqtt_handle.h"

/* --------------------- Definitions and static variables ------------------ */
/*TWAI*/
#define NO_OF_ITERS 3
#define RX_TASK_PRIO 9
#define TX_GPIO_NUM CONFIG_TWAI_TX_GPIO_NUM
#define RX_GPIO_NUM CONFIG_TWAI_RX_GPIO_NUM
#define TAG_TWAI "TWAI Listen Only"

#define ID_MASTER_STOP_CMD 0x0A0
#define ID_MASTER_START_CMD 0x0A1
#define ID_MASTER_PING 0x0A2
#define ID_SLAVE_STOP_RESP 0x0B0
#define ID_SLAVE_DATA 0x0B1
#define ID_SLAVE_PING_RESP 0x0B2

bool mqttConnected = 0;

// static const twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();
// static const twai_timing_config_t t_config = TWAI_TIMING_CONFIG_1MBITS();
// // Set TX queue length to 0 due to listen only mode
// static const twai_general_config_t g_config = {.mode = TWAI_MODE_LISTEN_ONLY,
//                                                .tx_io = TX_GPIO_NUM,
//                                                .rx_io = RX_GPIO_NUM,
//                                                .clkout_io = TWAI_IO_UNUSED,
//                                                .bus_off_io = TWAI_IO_UNUSED,
//                                                .tx_queue_len = 0,
//                                                .rx_queue_len = 5,
//                                                .alerts_enabled = TWAI_ALERT_NONE,
//                                                .clkout_divider = 0};
// static SemaphoreHandle_t rx_sem;

/*MQTT*/

static void log_error_if_nonzero(const char *message, int error_code)
{
  if (error_code != 0)
  {
    ESP_LOGE(TAG_MQTT, "Last error %s: 0x%x", message, error_code);
  }
}

//
static esp_mqtt5_user_property_item_t user_property_arr[] = ESP_MQTT5_USER_PROPERTY_ITEM();

static esp_mqtt5_publish_property_config_t publish_property = ESP_MQTT5_PUBLISH_PROPERTY_CONFIG();
static esp_mqtt5_subscribe_property_config_t subscribe_property = ESP_MQTT5_SUBSCRIBE_PROPERTY_CONFIG();
static esp_mqtt5_subscribe_property_config_t subscribe1_property = ESP_MQTT5_SUBSCRIBE1_PROPERTY_CONFIG();
static esp_mqtt5_unsubscribe_property_config_t unsubscribe_property = ESP_MQTT5_UNSUBSCRIBE_PROPERTY_CONFIG();
static esp_mqtt5_disconnect_property_config_t disconnect_property = ESP_MQTT5_DISCONNECT_PROPERTY_CONFIG();

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

/* --------- TWAI TASK BEGIN ---------*/

// static void twai_receive_task(void *arg)
// {
//   xSemaphoreTake(rx_sem, portMAX_DELAY);

//   while (1)
//   {
//     twai_message_t rx_msg;
//     twai_receive(&rx_msg, portMAX_DELAY);
//     uint32_t data = 0;
//     for (int i = 0; i < rx_msg.data_length_code; i++)
//     {
//       data |= (rx_msg.data[i] << (i * 8));
//     }
//     ESP_LOGI(TAG_TWAI, "Received %u data bytes with value %" PRIu32, rx_msg.data_length_code, data);
//     vTaskDelay(100);
//   }

//   xSemaphoreGive(rx_sem);
//   vTaskDelete(NULL);
// }
/* --------- TWAI TASK END ---------*/

/* --------- UART_1 SEND_DATA BEGIN ---------*/
// int sendData(const char *logName, const char *data)
// {
//   const int len = strlen(data);
//   const int txBytes = uart_write_bytes(UART_NUM_1, data, len);
//   ESP_LOGI(logName, "Wrote %d bytes", txBytes);
//   return txBytes;
// }
/* --------- UART_1 SEND_DATA END ---------*/

/* --------- UART_1 INIT BEGIN ---------*/
// void init(void)
// {
//   const uart_config_t uart_config = {
//       .baud_rate = 115200,
//       .data_bits = UART_DATA_8_BITS,
//       .parity = UART_PARITY_DISABLE,
//       .stop_bits = UART_STOP_BITS_1,
//       .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
//       .source_clk = UART_SCLK_DEFAULT,
//   };
//   // We won't use a buffer for sending data.
//   uart_driver_install(UART_NUM_1, RX_BUF_SIZE * 2, 0, 0, NULL, 0);
//   uart_param_config(UART_NUM_1, &uart_config);
//   uart_set_pin(UART_NUM_1, TXD_PIN, RXD_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
// }
/* --------- UART_1 INIT END ---------*/

/* --------- UART_1 CONFIGURE GPS BEGIN ---------*/
// void send_gps_task1(void)
// {
//   static const char *GPS_TASK_TAG = "GPS_TASK";
//   sendData(GPS_TASK_TAG, "AT\r\n");
//   vTaskDelay(1000 / portTICK_PERIOD_MS);
//   sendData(GPS_TASK_TAG, "AT\r\n");
//   vTaskDelay(1000 / portTICK_PERIOD_MS);
//   sendData(GPS_TASK_TAG, "AT\r\n");
//   vTaskDelay(1000 / portTICK_PERIOD_MS);

//   sendData(GPS_TASK_TAG, "AT+CGNSSPWR=1\r\n\r\n");
//   vTaskDelay(1000 / portTICK_PERIOD_MS);
//   sendData(GPS_TASK_TAG, "AT+CGPSHOT\r\n");
//   vTaskDelay(1000 / portTICK_PERIOD_MS);
//   sendData(GPS_TASK_TAG, "AT+CGNSSTST=1\r\n");
//   vTaskDelay(10000 / portTICK_PERIOD_MS);

//   sendData(GPS_TASK_TAG, "AT+CGPSINFO=1\r\n");
//   vTaskDelay(1000 / portTICK_PERIOD_MS);
//   sendData(GPS_TASK_TAG, "AT+CGNSSINFO=1\r\n");
//   vTaskDelay(1000 / portTICK_PERIOD_MS);
//   sendData(GPS_TASK_TAG, "AT+CGNSSPORTSWITCH=0,1\r\n");
//   vTaskDelay(1000 / portTICK_PERIOD_MS);
//   sendData(GPS_TASK_TAG, "AT+CGNSSNMEA=1,1,1,1,1,1,0,0,0,0\\r\n");
//   vTaskDelay(1000 / portTICK_PERIOD_MS);
// }
/* --------- UART_1 CONFIGURE GPS BEGIN ---------*/

/* --------- UART_1 TX_ASYNC_TASK BEGIN ---------*/
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
/* --------- UART_1 TX_ASYNC_TASK END ---------*/

/* --------- UART_1 TX_ASYNC_TASK BEGIN ---------*/
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
/* --------- UART_1 RX_ASYNC_TASK BEGIN ---------*/

/* --------- MQTT_EVENT_HANDLER BEGIN ---------*/
static void mqtt5_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
  ESP_LOGD(TAG_MQTT, "Event dispatched from event loop base=%s, event_id=%" PRIi32, base, event_id);
  esp_mqtt_event_handle_t event = event_data;
  esp_mqtt_client_handle_t client = event->client;
  ESP_LOGI(TAG_MQTT, "############## MQTT MQTT CLIENT: %p", client);

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
    mqttConnected = 1;

    // esp_mqtt5_client_set_user_property(&subscribe_property.user_property, user_property_arr, USE_PROPERTY_ARR_SIZE);
    // esp_mqtt5_client_set_subscribe_property(client, &subscribe_property);
    // msg_id = esp_mqtt_client_subscribe(client, "/topic/qos0", 0);
    // esp_mqtt5_client_delete_user_property(subscribe_property.user_property);
    // subscribe_property.user_property = NULL;
    // ESP_LOGI(TAG_MQTT, "sent subscribe successful, msg_id=%d", msg_id);

    // esp_mqtt5_client_set_user_property(&subscribe1_property.user_property, user_property_arr, USE_PROPERTY_ARR_SIZE);
    // esp_mqtt5_client_set_subscribe_property(client, &subscribe1_property);
    // msg_id = esp_mqtt_client_subscribe(client, "/topic/qos1", 2);
    // esp_mqtt5_client_delete_user_property(subscribe1_property.user_property);
    // subscribe1_property.user_property = NULL;
    // ESP_LOGI(TAG_MQTT, "sent subscribe successful, msg_id=%d", msg_id);

    // esp_mqtt5_client_set_user_property(&unsubscribe_property.user_property, user_property_arr, USE_PROPERTY_ARR_SIZE);
    // esp_mqtt5_client_set_unsubscribe_property(client, &unsubscribe_property);
    // msg_id = esp_mqtt_client_unsubscribe(client, "/topic/qos0");
    // ESP_LOGI(TAG_MQTT, "sent unsubscribe successful, msg_id=%d", msg_id);
    // esp_mqtt5_client_delete_user_property(unsubscribe_property.user_property);
    // unsubscribe_property.user_property = NULL;
    break;
  case MQTT_EVENT_DISCONNECTED:
    ESP_LOGI(TAG_MQTT, "MQTT_EVENT_DISCONNECTED");
    // print_user_property(event->property->user_property);
    break;
  case MQTT_EVENT_SUBSCRIBED:
    ESP_LOGI(TAG_MQTT, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
    // print_user_property(event->property->user_property);
    // esp_mqtt5_client_set_publish_property(client, &publish_property);
    // msg_id = esp_mqtt_client_publish(client, "/topic/qos0", "data", 0, 0, 0);
    // ESP_LOGI(TAG_MQTT, "sent publish successful, msg_id=%d", msg_id);
    break;
  case MQTT_EVENT_UNSUBSCRIBED:
    ESP_LOGI(TAG_MQTT, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
    // print_user_property(event->property->user_property);
    // esp_mqtt5_client_set_user_property(&disconnect_property.user_property, user_property_arr, USE_PROPERTY_ARR_SIZE);
    // esp_mqtt5_client_set_disconnect_property(client, &disconnect_property);
    // esp_mqtt5_client_delete_user_property(disconnect_property.user_property);
    // disconnect_property.user_property = NULL;
    // esp_mqtt_client_disconnect(client);
    break;
  case MQTT_EVENT_PUBLISHED:
    ESP_LOGI(TAG_MQTT, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
    // print_user_property(event->property->user_property);
    break;
  case MQTT_EVENT_DATA:
    ESP_LOGI(TAG_MQTT, "MQTT_EVENT_DATA");
    // print_user_property(event->property->user_property);
    ESP_LOGI(TAG_MQTT, "payload_format_indicator is %d", event->property->payload_format_indicator);
    ESP_LOGI(TAG_MQTT, "response_topic is %.*s", event->property->response_topic_len, event->property->response_topic);
    ESP_LOGI(TAG_MQTT, "correlation_data is %.*s", event->property->correlation_data_len, event->property->correlation_data);
    ESP_LOGI(TAG_MQTT, "content_type is %.*s", event->property->content_type_len, event->property->content_type);
    ESP_LOGI(TAG_MQTT, "TOPIC=%.*s", event->topic_len, event->topic);
    ESP_LOGI(TAG_MQTT, "DATA=%.*s", event->data_len, event->data);
    break;
  case MQTT_EVENT_ERROR:
    ESP_LOGI(TAG_MQTT, "MQTT_EVENT_ERROR");
    // print_user_property(event->property->user_property);
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
/* --------- MQTT_EVENT_HANDLER END ---------*/

/* --------- GPS_EVENT_HANDLER BEGIN ---------*/
static void gps_event_handler(void *handler_args, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
  gps_t *gps = NULL;
  esp_mqtt_client_handle_t esp_mqtt_client = handler_args;
  ESP_LOGI(TAG_MQTT, "############## MQTT GPS CLIENT: %p", esp_mqtt_client);

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

    //  char buffer;
    //  int num = 42;
    //  sprintf(buffer, "The number is %d", num);

    // sprintf(buffer, "\"year\": %d, \"month\": %d, \"day\": %d,
    //   \"hour\": %d, \"minute\": %d, \"second\": %d,
    //   \"latitude\": %.05f°N, \"longitude\": %.05f°E, \"altitude\": %.02fm, \"speed\": %fm/s",
    // gps->date.year + YEAR_BASE, gps->date.month, gps->date.day,
    //  gps->tim.hour + TIME_ZONE, gps->tim.minute, gps->tim.second,
    //  gps->latitude, gps->longitude, gps->altitude, gps->speed);

    // // /* data buffers */
    // uint8_t buff_up[1024]; /* buffer to compose the upstream packet */
    // int buff_index = 0;
    // // uint8_t buff_ack[32];
    // /* start of JSON structure */
    // memcpy((void *)(buff_up + buff_index), (void *)"{\"rxpk\":[", 9);

    if (esp_mqtt_client)
    {
      printf("MQTT CONNECTED?: %d", mqttConnected);
      int msg_id = esp_mqtt_client_publish(esp_mqtt_client, "OpenDataTelemetry/FSAELive/IC/001/rx", "{\"key\":\"value\"}", 0, 1, 0);
    }

    break;
  case GPS_UNKNOWN:
    /* print unknown statements */
    ESP_LOGW(TAG_GPS, "Unknown statement:%s", (char *)event_data);
    break;
  default:
    break;
  }
}

/* --------- TWAI_EVENT_HANDLER BEGIN ---------*/
static void twai_event_handler(void *handler_args, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
  // gps_t *twai = NULL;
  esp_mqtt_client_handle_t esp_mqtt_client = handler_args;
  ESP_LOGI(TAG_MQTT, "############## MQTT GPS CLIENT: %p", esp_mqtt_client);

  switch (event_id)
  {
  case TWAI_UPDATE:
  ESP_LOGW(TAG_TWAI, "TWAI_UPDATE");

  // twai = (twai_t *)event_data;
  //   /* print information parsed from GPS statements */
  //   // ESP_LOGI(TAG_GPS, "%d/%d/%d %d:%d:%d => \r\n"
  //   //                   "\t\t\t\t\t\tlatitude   = %.05f°N\r\n"
  //   //                   "\t\t\t\t\t\tlongitude = %.05f°E\r\n"
  //   //                   "\t\t\t\t\t\taltitude   = %.02fm\r\n"
  //   //                   "\t\t\t\t\t\tspeed      = %fm/s",
  //   //          gps->date.year + YEAR_BASE, gps->date.month, gps->date.day,
  //   //          gps->tim.hour + TIME_ZONE, gps->tim.minute, gps->tim.second,
  //   //          gps->latitude, gps->longitude, gps->altitude, gps->speed);

  //   if (esp_mqtt_client)
  //   {
  //     printf("MQTT CONNECTED?: %d", mqttConnected);
  //     int msg_id = esp_mqtt_client_publish(esp_mqtt_client, "OpenDataTelemetry/FSAELive/IC/001/rx", "{\"key\":\"twai\"}", 0, 1, 0);
  //   }

    break;
  case TWAI_UNKNOWN:
    /* print unknown statements */
    ESP_LOGW(TAG_TWAI, "Unknown statement:%s", (char *)event_data);
    break;
  default:
    break;
  }
}
/* --------- GPS_EVENT_HANDLER END ---------*/

void app_main(void)
{
  ESP_LOGI(TAG_APP, "[APP] Startup..");
  ESP_LOGI(TAG_APP, "[APP] Free memory: %" PRIu32 " bytes", esp_get_free_heap_size());
  ESP_LOGI(TAG_APP, "[APP] IDF version: %s", esp_get_idf_version());
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

  /* --------- MQTT BEGIN ---------*/

  // mqtt5_app_start();
  // esp_mqtt5_connection_property_config_t connect_property = ESP_MQTT5_CONNECTION_PROPERTY_CONFIG();
  // esp_mqtt_client_config_t esp_mqtt_client_config = ESP_MQTT_CLIENT_CONFIG_DEFAULT();
  // esp_mqtt_client_handle_t esp_mqtt_client = esp_mqtt_client_init(&esp_mqtt_client_config);
  // esp_mqtt5_client_set_user_property(&connect_property.user_property, user_property_arr, USE_PROPERTY_ARR_SIZE);
  // esp_mqtt5_client_set_user_property(&connect_property.will_user_property, user_property_arr, USE_PROPERTY_ARR_SIZE);
  // esp_mqtt5_client_set_connect_property(esp_mqtt_client, &connect_property);

  /* If you call esp_mqtt5_client_set_user_property to set user properties, DO NOT forget to delete them.
   * esp_mqtt5_client_set_connect_property will malloc buffer to store the user_property and you can delete it after
   */
  // esp_mqtt5_client_delete_user_property(connect_property.user_property);
  // esp_mqtt5_client_delete_user_property(connect_property.will_user_property);

  /* The last argument may be used to pass data to the event handler, in this example mqtt_event_handler */
  // esp_mqtt_client_register_event(esp_mqtt_client, ESP_EVENT_ANY_ID, mqtt5_event_handler, NULL);
  // esp_mqtt_client_start(esp_mqtt_client);
  /* --------- MQTT END ---------*/



  // init();
  // xTaskCreate(uart_rx_async_task, "uart_rx_task", 2048 * 2, NULL, configMAX_PRIORITIES - 1, NULL);
  // xTaskCreate(uart_tx_async_task, "uart_tx_async_task", 2048 * 2, NULL, configMAX_PRIORITIES - 2, NULL);

  // /*CAN*/
  // /* --------- TWAI BEGIN ---------*/
  // rx_sem = xSemaphoreCreateBinary();
  // xTaskCreatePinnedToCore(twai_receive_task, "TWAI_rx", 4096, NULL, RX_TASK_PRIO, NULL, tskNO_AFFINITY);

  // // Install and start TWAI driver
  // ESP_ERROR_CHECK(twai_driver_install(&g_config, &t_config, &f_config));
  // ESP_LOGI(TAG_TWAI, "Driver installed");
  // ESP_ERROR_CHECK(twai_start());
  // ESP_LOGI(TAG_TWAI, "Driver started");

  // xSemaphoreGive(rx_sem); // Start RX task
  // vTaskDelay(pdMS_TO_TICKS(100));
  // xSemaphoreTake(rx_sem, portMAX_DELAY); // Wait for RX task to complete

  // // Stop and uninstall TWAI driver
  // ESP_ERROR_CHECK(twai_stop());
  // ESP_LOGI(TAG_TWAI, "Driver stopped");
  // ESP_ERROR_CHECK(twai_driver_uninstall());
  // ESP_LOGI(TAG_TWAI, "Driver uninstalled");

  twai_config_t twai_config = TWAI_CONFIG_DEFAULT();
  twai_handle_t twai = twai_init(&twai_config);
  twai_register_event(twai, ESP_EVENT_ANY_ID, twai_event_handler, NULL);
  // twai_register_event(twai, ESP_EVENT_ANY_ID, twai_event_handler, esp_mqtt_client);
  /* --------- TWAI END ---------*/
  

  
  /* --------- UART_1 NMEA BEGIN ---------*/
  // nmea_parser_config_t nmea_parser_config = NMEA_PARSER_CONFIG_DEFAULT();
  // nmea_parser_handle_t nmea_parser = nmea_parser_init(&nmea_parser_config);
  // nmea_parser_register_event(nmea_parser, ESP_EVENT_ANY_ID, gps_event_handler, esp_mqtt_client);

  // vTaskDelay(10000 / portTICK_PERIOD_MS);
  /* --------- UART_1 NMEA END ---------*/

  // xTaskCreatePinnedToCore(twai_receive_task, "TWAI_rx", 4096, NULL, RX_TASK_PRIO, NULL, tskNO_AFFINITY);
  // Cleanup
  // vSemaphoreDelete(rx_sem);
  /* unregister event handler */
  // nmea_parser_remove_handler(nmea_hdl, gps_event_handler);
  /* deinit NMEA parser library */
  // nmea_parser_deinit(nmea_hdl);


  // ESP_ERROR_CHECK(example_connect());
}
