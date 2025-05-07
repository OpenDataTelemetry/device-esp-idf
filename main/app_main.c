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

/*CAN*/
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "esp_err.h"
#include "driver/twai.h"
#include "twai.h"

/*MQTT*/
// #include <stdint.h>
// #include <stddef.h>
// #include <string.h>
// #include "nvs_flash.h"
// #include "esp_event.h"
// #include "esp_netif.h"
// #include "protocol_examples_common.h"
// #include "mqtt_client.h"
#include "esp_mqtt_handle.h"

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "protocol_examples_common.h"

#include "esp_log.h"
#include "mqtt_client.h"

/*NMEA*/
// #include "nmea_parser.h"

static const char *TAG_APP = "app_main";
static const char *TAG_GPS = "app_main_gps";
static const char *TAG_MQTT = "app_main_mqtt5";

#define TIME_ZONE (0)    // Beijing Time
#define YEAR_BASE (2000) // date in GPS starts from 2000

/*UART ASYNC*/
// static const int RX_BUF_SIZE = 2048;
#define TXD_PIN (GPIO_NUM_18)
#define RXD_PIN (GPIO_NUM_17)

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

/*MQTT*/

static void log_error_if_nonzero(const char *message, int error_code)
{
  if (error_code != 0)
  {
    ESP_LOGE(TAG_MQTT, "Last error %s: 0x%x", message, error_code);
  }
}


/* --------------------------- Tasks and Functions -------------------------- */

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
// static void uart_rx_async_task(void *arg)

// {
//   static const char *RX_TASK_TAG = "RX_TASK";
//   esp_log_level_set(RX_TASK_TAG, ESP_LOG_INFO);
//   uint8_t *data = (uint8_t *)malloc(RX_BUF_SIZE + 1);
//   while (1)
//   {
//     const int rxBytes = uart_read_bytes(UART_NUM_1, data, RX_BUF_SIZE, 1000 / portTICK_PERIOD_MS);
//     if (rxBytes > 0)
//     {
//       data[rxBytes] = 0;
//       ESP_LOGI(RX_TASK_TAG, "Read %d bytes: '%s'", rxBytes, data);
//       ESP_LOG_BUFFER_HEXDUMP(RX_TASK_TAG, data, rxBytes, ESP_LOG_INFO);
//     }
//   }
//   free(data);
// }
/* --------- UART_1 RX_ASYNC_TASK BEGIN ---------*/

/* --------- EVENT HANDLERS BEGIN ---------*/

// static void gps_event_handler(void *handler_args, esp_event_base_t event_base, int32_t event_id, void *event_data)
// {
//   gps_t *gps = NULL;
//   esp_mqtt_client_handle_t esp_mqtt_client = handler_args;

//   switch (event_id)
//   {
//   case GPS_UPDATE:
//     gps = (gps_t *)event_data;
//     /* print information parsed from GPS statements */
//     ESP_LOGI(TAG_GPS, "%d/%d/%d %d:%d:%d => \r\n"
//                       "\t\t\t\t\t\tlatitude   = %.05f°N\r\n"
//                       "\t\t\t\t\t\tlongitude = %.05f°E\r\n"
//                       "\t\t\t\t\t\taltitude   = %.02fm\r\n"
//                       "\t\t\t\t\t\tspeed      = %fm/s",
//              gps->date.year + YEAR_BASE, gps->date.month, gps->date.day,
//              gps->tim.hour + TIME_ZONE, gps->tim.minute, gps->tim.second,
//              gps->latitude, gps->longitude, gps->altitude, gps->speed);

//   // TODO: Mount the GPS message payload

//     if (esp_mqtt_client)
//     {
//       int msg_id = esp_mqtt_client_publish(esp_mqtt_client, "OpenDataTelemetry/FSAELive/IC/001/rx", "{\"key\":\"value\"}", 0, 1, 0);
//     }

//     break;
//   case GPS_UNKNOWN:
//     /* print unknown statements */
//     ESP_LOGW(TAG_GPS, "Unknown statement:%s", (char *)event_data);
//     break;
//   default:
//     break;
//   }
// }

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    ESP_LOGD(TAG_MQTT, "Event dispatched from event loop base=%s, event_id=%" PRIi32 "", base, event_id);
    esp_mqtt_event_handle_t event = event_data;
    esp_mqtt_client_handle_t client = event->client;
    int msg_id;
    switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG_MQTT, "MQTT_EVENT_CONNECTED");
        msg_id = esp_mqtt_client_publish(client, "/topic/qos1", "data_3", 0, 1, 0);
        ESP_LOGI(TAG_MQTT, "sent publish successful, msg_id=%d", msg_id);

        msg_id = esp_mqtt_client_subscribe(client, "/topic/qos0", 0);
        ESP_LOGI(TAG_MQTT, "sent subscribe successful, msg_id=%d", msg_id);

        msg_id = esp_mqtt_client_subscribe(client, "/topic/qos1", 1);
        ESP_LOGI(TAG_MQTT, "sent subscribe successful, msg_id=%d", msg_id);

        msg_id = esp_mqtt_client_unsubscribe(client, "/topic/qos1");
        ESP_LOGI(TAG_MQTT, "sent unsubscribe successful, msg_id=%d", msg_id);
        break;
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(TAG_MQTT, "MQTT_EVENT_DISCONNECTED");
        break;

    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGI(TAG_MQTT, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
        msg_id = esp_mqtt_client_publish(client, "/topic/qos0", "data", 0, 0, 0);
        ESP_LOGI(TAG_MQTT, "sent publish successful, msg_id=%d", msg_id);
        break;
    case MQTT_EVENT_UNSUBSCRIBED:
        ESP_LOGI(TAG_MQTT, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_PUBLISHED:
        ESP_LOGI(TAG_MQTT, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_DATA:
        ESP_LOGI(TAG_MQTT, "MQTT_EVENT_DATA");
        printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
        printf("DATA=%.*s\r\n", event->data_len, event->data);
        break;
    case MQTT_EVENT_ERROR:
        ESP_LOGI(TAG_MQTT, "MQTT_EVENT_ERROR");
        if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
            log_error_if_nonzero("reported from esp-tls", event->error_handle->esp_tls_last_esp_err);
            log_error_if_nonzero("reported from tls stack", event->error_handle->esp_tls_stack_err);
            log_error_if_nonzero("captured as transport's socket errno",  event->error_handle->esp_transport_sock_errno);
            ESP_LOGI(TAG_MQTT, "Last errno string (%s)", strerror(event->error_handle->esp_transport_sock_errno));

        }
        break;
    default:
        ESP_LOGI(TAG_MQTT, "Other event id:%d", event->event_id);
        break;
    }
}


static void twai_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
  // gps_t *twai = NULL;
  // esp_mqtt_client_handle_t esp_mqtt_client = handler_args;
  // ESP_LOGI(TAG_MQTT, "############## MQTT GPS CLIENT: %p", esp_mqtt_client);

  // switch (event_id)
  switch ((esp_twai_event_id_t)event_id)
  {
  case TWAI_EVENT_CONNECTED:
    ESP_LOGW(TAG_TWAI, "TWAI_EVENT_CONNECTED");

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
  case TWAI_EVENT_DATA:
    /* print unknown statements */
    ESP_LOGW(TAG_TWAI, "Unknown statement:%s", (char *)event_data);
    break;
  default:
    break;
  }
}
/* --------- EVENT HANDLERS END ---------*/

static void mqtt_app_start(void)
{
  ESP_LOGI(TAG_APP, "[APP] MQTT START..");
  esp_mqtt_client_config_t config = ESP_MQTT_CLIENT_CONFIG_DEFAULT();
  esp_mqtt_client_handle_t client = esp_mqtt_client_init(&config);
  esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
  ESP_LOGI(TAG_APP, "[APP] MQTT CLIENT OK..");

  esp_mqtt_client_start(client);
}

static void twai_app_start(void)
{
  ESP_LOGI(TAG_APP, "[APP] TWAI START..");
  esp_twai_client_config_t config = ESP_TWAI_CLIENT_CONFIG_DEFAULT();
  esp_twai_client_handle_t client = esp_twai_client_init(&config);
  esp_twai_client_register_event(client, ESP_EVENT_ANY_ID, twai_event_handler, NULL);
  ESP_LOGI(TAG_APP, "[APP] TWAI CLIENT OK..");

  esp_twai_client_start(client);
}

void app_main(void)
{
  /* --------- APP Main Init ---------*/
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



  // ESP_ERROR_CHECK(example_connect());

  // mqtt_app_start();
  twai_app_start();


  // init();
  // xTaskCreate(uart_rx_async_task, "uart_rx_task", 2048 * 2, NULL, configMAX_PRIORITIES - 1, NULL);
  // xTaskCreate(uart_tx_async_task, "uart_tx_async_task", 2048 * 2, NULL, configMAX_PRIORITIES - 2, NULL);

  /* --------- UART_1 NMEA BEGIN ---------*/
  // TO DO: MODIFY NMEA_PARSER TO UART_1
  //  UART_1 SHALL CONFIGURE THE UART PARAMETERS, THEN SET THE MODEM TO RECEIVE GPS AND TO CONNECT TO MQTT5_ADMINISTRATIVE_ACTION
  //  THE UART_1 EVENT_LOOP SHALL CONSIDER TO RECEIVE THE NMEA STRING AND TO SEND TO THE MQTT VIA MODEM_REQUIRED_MIN_APB_CLK_FREQ

  // UART_1 -> MODEM -> 1. SET AT COMMAND TO RECEIVE GNSS, 2. SET COMMAND TO CONNECT MQTT, 3. CREATE AN EVENT LOOP TO RECEIVE THE GNSS, 4. CREATE AN EVENT LOOP TO CONNECT MODEM VIA MQTT
  // ESP_LOGI(TAG_APP, "[APP] NMEA BEGIN..");
  // uart_1_config_t uart_1_config = NMEA_PARSER_CONFIG_DEFAULT();
  // uart_1_handle_t uart_1_handle = uart_1_init(&uart_1_config);
  // // nmea_parser_register_event(uart_1__handle, ESP_EVENT_ANY_ID, gps_event_handler, esp_mqtt_client_handle);
  // nmea_parser_register_event(uart_1_handle, ESP_EVENT_ANY_ID, gps_event_handler, NULL);
  // nmea_parser_start(uart_1_handle);

  // ESP_LOGI(TAG_APP, "[APP] BEFORE NMEA vTaskDelay");
  // vTaskDelay(10000 / portTICK_PERIOD_MS);
  // ESP_LOGI(TAG_APP, "[APP] AFTER NMEA vTaskDelay");
  // nmea_parser_remove_handler(nmea_parser, gps_event_handler);
  // nmea_parser_deinit(nmea_parser);

  /* --------- TWAI BEGIN ---------*/
  // ESP_LOGI(TAG_APP, "[APP] TWAI BEGIN..");
  // esp_twai_client_config_t esp_twai_client_config = ESP_TWAI_CLIENT_CONFIG_DEFAULT();
  // esp_twai_client_handle_t esp_twai_client_handle = esp_twai_client_init(&esp_twai_client_config);
  // esp_twai_client_register_event(esp_twai_client_handle, ESP_EVENT_ANY_ID, twai_event_handler, NULL);
  // esp_twai_client_register_event(twai, ESP_EVENT_ANY_ID, twai_event_handler, esp_mqtt_client);
  // esp_twai_client_start(twai_client_handle);

  // ESP_ERROR_CHECK(example_connect());
}
