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

#include <stdio.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "esp_err.h"
#include "esp_log.h"
#include "driver/twai.h"

#include <stdint.h>
#include <stdlib.h>
#include "esp_err.h"
#include "esp_log.h"
#include "esp_heap_caps.h"
#include "esp_transport.h"
// #include "mqtt_client.h"
// #include "mqtt_client_priv.h"
// #include "mqtt_msg.h"
// #include "mqtt_outbox.h"

// #include <stdio.h>
// #include <stdlib.h>
// #include "freertos/FreeRTOS.h"
// #include "freertos/task.h"
// #include "freertos/queue.h"
// #include "freertos/semphr.h"
// #include "esp_err.h"
// #include "esp_log.h"
// #include "driver/twai.h"
// #include "twai.h"

/*TWAI*/
#define NO_OF_ITERS 3`
#define RX_TASK_PRIO 9

#define TAG_TWAI "TWAI Listen Only"

#define ID_MASTER_STOP_CMD 0x0A0
#define ID_MASTER_START_CMD 0x0A1
#define ID_MASTER_PING 0x0A2
#define ID_SLAVE_STOP_RESP 0x0B0
#define ID_SLAVE_DATA 0x0B1
#define ID_SLAVE_PING_RESP 0x0B2

const static int STOPPED_BIT = (1 << 0);
const static int RECONNECT_BIT = (1 << 1);
const static int DISCONNECT_BIT = (1 << 2);

ESP_EVENT_DEFINE_BASE(TWAI_EVENTS);

static SemaphoreHandle_t rx_sem;

void twai_msg_buffer_destroy(twai_connection_t *connection)
{
  if (connection)
  {
    free(connection->buffer);
  }
}

esp_err_t esp_twai_set_config(esp_twai_client_handle_t client, const esp_twai_client_config_t *config)
{
  if (!client)
  {
    ESP_LOGE(TAG_TWAI, "Client was not initialized");
    return ESP_ERR_INVALID_ARG;
  }
  ESP_LOGI(TAG_TWAI, "Client shall be initialized");

  TWAI_API_LOCK(client);
  ESP_LOGI(TAG_TWAI, "TWAI LOCKED");

  // Copy user configurations to client context
  esp_err_t err = ESP_OK;
  if (!client->config)
  {
    ESP_LOGI(TAG_TWAI, "client->config NOT OK! CALLOC client->config");
    client->config = calloc(1, sizeof(twai_config_storage_t));
  }

  ESP_LOGI(TAG_TWAI, "DEFINE TWAI_TASK_PRIORITY");
  client->config->task_prio = config->task.priority;
  if (client->config->task_prio <= 0)
  {
    client->config->task_prio = TWAI_TASK_PRIORITY;
  }

  ESP_LOGI(TAG_TWAI, "DEFINE TWAI_TASK_STACK");
  client->config->task_stack = config->task.stack_size;
  if (client->config->task_stack <= 0)
  {
    client->config->task_stack = TWAI_TASK_STACK;
  }

  ESP_LOGI(TAG_TWAI, "DEFINE TWAI_DRIVER");
  client->config->driver_f_config = config->f_config;
  client->config->driver_g_config = config->g_config;
  client->config->driver_t_config = config->t_config;

  ESP_LOGI(TAG_TWAI, "PRIORITY AND STACK DEFINED");

  err = ESP_ERR_NO_MEM;
  esp_err_t config_has_conflict = 0;

  TWAI_API_UNLOCK(client);
  ESP_LOGI(TAG_TWAI, "config_has_conflict: %u", config_has_conflict);
  return config_has_conflict;
}

void esp_twai_destroy_config(esp_twai_client_handle_t client)
{
  if (client->config == NULL)
  {
    return;
  }
  free(client->twai_state.in_buffer);
  twai_msg_buffer_destroy(&client->twai_state.connection);
  // for (int i = 0; i < client->config->num_alpn_protos; i++) {
  //     free(client->config->alpn_protos[i]);
  // }

  memset(client->config, 0, sizeof(twai_config_storage_t));
  free(client->config);
  client->config = NULL;
}

static bool create_client_data(esp_twai_client_handle_t client)
{
  client->event.error_handle = calloc(1, sizeof(esp_twai_error_codes_t));
  // ESP_MEM_CHECK(TAG_TWAI, client->event.error_handle, return false)

  client->api_lock = xSemaphoreCreateRecursiveMutex();
  // ESP_MEM_CHECK(TAG_TWAI, client->api_lock, return false);

  // client->outbox = outbox_init();
  // ESP_MEM_CHECK(TAG_TWAI, client->outbox, return false);
  client->status_bits = xEventGroupCreate();
  // ESP_MEM_CHECK(TAG_TWAI, client->status_bits, return false);

  return true;
}

esp_twai_client_handle_t esp_twai_client_init(const esp_twai_client_config_t *config)
{
  esp_twai_client_handle_t client = heap_caps_calloc(1, sizeof(struct esp_twai_client), MALLOC_CAP_DEFAULT);

  if (!create_client_data(client))
  {
    ESP_LOGE(TAG_TWAI, "Error in create_client_data");
  }

  if (esp_twai_set_config(client, config) != ESP_OK)
  {
    ESP_LOGE(TAG_TWAI, "Error in esp_twai_set_config");
  }

  // JUMPED ESP_MEM_CHECK
  esp_event_loop_args_t no_task_loop = {
      .queue_size = 1,
      .task_name = NULL,
  };
  esp_event_loop_create(&no_task_loop, &client->config->event_loop_handle);

  // client->keepalive_tick = platform_tick_get_ms();
  // client->reconnect_tick = platform_tick_get_ms();
  // client->refresh_connection_tick = platform_tick_get_ms();
  // client->wait_for_ping_resp = false;

  return client;
  // _twai_init_failed:
  //   esp_twai_client_destroy(client);
  //   return NULL;

  //   rx_sem = xSemaphoreCreateBinary();
  //   xTaskCreatePinnedToCore(twai_task, "TWAI_rx", 4096, NULL, RX_TASK_PRIO, NULL, tskNO_AFFINITY);

  //   xSemaphoreGive(rx_sem); // Start RX task
  //   vTaskDelay(pdMS_TO_TICKS(100));
  //   xSemaphoreTake(rx_sem, portMAX_DELAY); // Wait for RX task to complete

  //   // Stop and uninstall TWAI driver
  //   ESP_ERROR_CHECK(twai_stop());
  //   ESP_LOGI(TAG_TWAI, "Driver stopped");
  //   ESP_ERROR_CHECK(twai_driver_uninstall());
  //   ESP_LOGI(TAG_TWAI, "Driver uninstalled");

  //   // Cleanup
  //   vSemaphoreDelete(rx_sem);

  //   return NULL;
}

// esp_err_t esp_twai_client_destroy(esp_twai_client_handle_t client)
// {
//   if (client == NULL)
//   {
//     return ESP_ERR_INVALID_ARG;
//   }
//   if (client->run)
//   {
//     esp_twai_client_stop(client);
//     if (client->status_bits)
//     {
//       vEventGroupDelete(client->status_bits);
//     }
//     if (client->api_lock)
//     {
//       vSemaphoreDelete(client->api_lock);
//     }
//     free(client->event.error_handle);
//     free(client);
//     return ESP_OK;
//   }
// }

static esp_err_t esp_twai_dispatch_event(esp_twai_client_handle_t client)
{
  client->event.client = client;
  esp_err_t ret = ESP_FAIL;

  esp_event_post_to(client->config->event_loop_handle, TWAI_EVENTS, client->event.event_id, &client->event, sizeof(client->event), portMAX_DELAY);
  ret = esp_event_loop_run(client->config->event_loop_handle, 0);

  return ret;
}

static esp_err_t deliver_publish(esp_twai_client_handle_t client)
{
  // uint8_t *msg_buf = client->twai_state.in_buffer;
  // size_t msg_read_len = client->twai_state.in_buffer_read_len;
  // size_t msg_total_len = client->twai_state.message_length;
  // size_t msg_topic_len = msg_read_len, msg_data_len = msg_read_len;
  // size_t msg_data_offset = 0;
  // char *msg_topic = NULL, *msg_data = NULL;

  // // get topic
  // msg_topic = mqtt_get_publish_topic(msg_buf, &msg_topic_len);
  // if (msg_topic == NULL) {
  //     ESP_LOGE(TAG_TWAI, "%s: mqtt_get_publish_topic() failed", __func__);
  //     return ESP_FAIL;
  // }
  // ESP_LOGD(TAG_TWAI, "%s: msg_topic_len=%"NEWLIB_NANO_COMPAT_FORMAT, __func__, NEWLIB_NANO_COMPAT_CAST(msg_topic_len));

  // // get payload
  // msg_data = mqtt_get_publish_data(msg_buf, &msg_data_len);
  // if (msg_data_len > 0 && msg_data == NULL) {
  //     ESP_LOGE(TAG_TWAI, "%s: mqtt_get_publish_data() failed", __func__);
  //     return ESP_FAIL;
  // }

  // post data event
  // client->event.retain = mqtt_get_retain(msg_buf);
  //     client->event.msg_id = mqtt_get_id(msg_buf, msg_read_len);

  // client->event.total_data_len = msg_data_len + msg_total_len - msg_read_len;
  // post_data_event:
  //     ESP_LOGD(TAG_TWAI, "Get data len= %"NEWLIB_NANO_COMPAT_FORMAT", topic len=%"NEWLIB_NANO_COMPAT_FORMAT", total_data: %d offset: %"NEWLIB_NANO_COMPAT_FORMAT,
  //              NEWLIB_NANO_COMPAT_CAST(msg_data_len), NEWLIB_NANO_COMPAT_CAST(msg_topic_len),
  //              client->event.total_data_len, NEWLIB_NANO_COMPAT_CAST(msg_data_offset));
  //     client->event.event_id = TWAI_EVENT_DATA;
  //     client->event.data = msg_data_len > 0 ? msg_data : NULL;
  //     client->event.data_len = msg_data_len;
  //     client->event.current_data_offset = msg_data_offset;
  //     client->event.topic = msg_topic;
  //     client->event.topic_len = msg_topic_len;
  //     esp_mqtt_dispatch_event(client);

  // if (msg_read_len < msg_total_len) {
  //     size_t buf_len = client->mqtt_state.in_buffer_length;

  //     msg_data = (char *)client->mqtt_state.in_buffer;
  //     msg_topic = NULL;
  //     msg_topic_len = 0;
  //     msg_data_offset += msg_data_len;
  //     int ret = esp_transport_read(client->transport, (char *)client->mqtt_state.in_buffer,
  //                                  msg_total_len - msg_read_len > buf_len ? buf_len : msg_total_len - msg_read_len,
  //                                  client->config->network_timeout_ms);
  //     if (ret <= 0) {
  //         return esp_mqtt_handle_transport_read_error(ret, client, false) == 0 ? ESP_OK : ESP_FAIL;
  //     }

  //     msg_data_len = ret;
  //     msg_read_len += msg_data_len;
  //     goto post_data_event;
  // }
  return ESP_OK;
}

static inline void run_event_loop(esp_twai_client_handle_t client)
{
  esp_err_t ret = esp_event_loop_run(client->config->event_loop_handle, 0);
  if (ret != ESP_OK)
  {
    ESP_LOGE(TAG_TWAI, "Error in running event_loop %d", ret);
  }
}

static void esp_twai_task(void *arg)
{
  ESP_LOGI(TAG_TWAI, "esp_twai_task()");

  xSemaphoreTake(rx_sem, portMAX_DELAY);
  ESP_LOGI(TAG_TWAI, "xSemaphoreTake()");

  while (1)
  {
    twai_message_t rx_msg;
    twai_receive(&rx_msg, portMAX_DELAY);
    unsigned long long data = 0;
    for (int i = 0; i < rx_msg.data_length_code; i++)
    {
      data = data << 8 | rx_msg.data[i];
    }
    ESP_LOGI(TAG_TWAI, "Id: %lu, Msg: %llu", rx_msg.identifier, data);
    // base64_encode(data, buffer);
    vTaskDelay(100);
  }

  xSemaphoreGive(rx_sem);
  vTaskDelete(NULL);
}

static void esp_twai_task2(void *pv)
{
  esp_twai_client_handle_t client = (esp_twai_client_handle_t)pv;
  uint64_t last_retransmit = 0;
  client->run = true;
  // xSemaphoreTake(rx_sem, portMAX_DELAY);

  client->state = TWAI_STATE_INIT;
  xEventGroupClearBits(client->status_bits, STOPPED_BIT);
  while (client->run)
  {
    TWAI_API_LOCK(client);
    run_event_loop(client);
    // delete long pending messages
    twai_client_state_t state = client->state;
    switch (state)
    {
    case TWAI_STATE_DISCONNECTED:
      break;
    case TWAI_STATE_INIT:
      xEventGroupClearBits(client->status_bits, RECONNECT_BIT | DISCONNECT_BIT);
      client->event.event_id = TWAI_EVENT_BEFORE_CONNECT;
      esp_twai_dispatch_event(client);

      client->event.event_id = TWAI_EVENT_CONNECTED;
      client->state = TWAI_STATE_CONNECTED;
      esp_twai_dispatch_event(client);

        // Install and start TWAI driver
      ESP_ERROR_CHECK(twai_driver_install(&client->config->driver_g_config, &client->config->driver_t_config, &client->config->driver_f_config));
      ESP_LOGI(TAG_TWAI, "Driver installed");
      ESP_ERROR_CHECK(twai_start());
      ESP_LOGI(TAG_TWAI, "Driver started");

      break;
    case TWAI_STATE_CONNECTED:
      //   // check for disconnection request
      //   if (xEventGroupWaitBits(client->status_bits, DISCONNECT_BIT, true, true, 0) & DISCONNECT_BIT)
      //   {
      //     send_disconnect_msg(client); // ignore error, if clean disconnect fails, just abort the connection
      //     break;
      //   }

      //   if (last_retransmit == 0)
      //   {
      //     // connected for first time, set last_retransmit to now, avoid retransmit
      //     last_retransmit = platform_tick_get_ms();
      //   }

      twai_message_t rx_msg;
      twai_receive(&rx_msg, portMAX_DELAY);
      unsigned long long data = 0;
      for (int i = 0; i < rx_msg.data_length_code; i++)
      {
        data = data << 8 | rx_msg.data[i];
      }
      ESP_LOGI(TAG_TWAI, "Id: %lu, Msg: %llu", rx_msg.identifier, data);
      vTaskDelay(100);
      break;

      TWAI_API_UNLOCK(client);
      // xEventGroupWaitBits(client->status_bits, RECONNECT_BIT, false, true,
      //                     max_poll_timeout(client, client->wait_timeout_ms / 2 / portTICK_PERIOD_MS));
      // continue the while loop instead of break, as the mutex is unlocked
      continue;
    default:
      ESP_LOGE(TAG_TWAI, "MQTT client error, client is in an unrecoverable state.");
      break;
    }
    TWAI_API_UNLOCK(client);
  }

  xEventGroupSetBits(client->status_bits, STOPPED_BIT);
  client->state = TWAI_STATE_DISCONNECTED;

  vTaskDelete(NULL);
}

esp_err_t esp_twai_client_start(esp_twai_client_handle_t client)
{
  if (!client)
  {
    ESP_LOGI(TAG_TWAI, "Client was not initialized");
    return ESP_ERR_INVALID_ARG;
  }
  ESP_LOGI(TAG_TWAI, "TWAI_API_LOCK");
  TWAI_API_LOCK(client);
  ESP_LOGI(TAG_TWAI, "client->state %u", client->state);

  if (client->state != TWAI_STATE_INIT && client->state != TWAI_STATE_DISCONNECTED)
  {
    ESP_LOGI(TAG_TWAI, "Client has started");
    TWAI_API_UNLOCK(client);
    return ESP_FAIL;
  }
  ESP_LOGI(TAG_TWAI, "!TWAI_STATE_INIT AND !TWAI_STATE_DISCONNECTED");

  esp_err_t err = ESP_OK;

  ESP_LOGD(TAG_TWAI, "Core selection disabled");
  if (xTaskCreate(esp_twai_task2, "twai_task", client->config->task_stack, client, client->config->task_prio, &client->task_handle) != pdTRUE)
  {
    ESP_LOGE(TAG_TWAI, "Error create TWAI task");
    err = ESP_FAIL;
  }

  TWAI_API_UNLOCK(client);
  return err;
}

// twai_handle_t twai_start(const twai_config_t *twai_config)
// {
//   rx_sem = xSemaphoreCreateBinary();
//   xTaskCreatePinnedToCore(twai_receive_task, "TWAI_rx", 4096, NULL, RX_TASK_PRIO, NULL, tskNO_AFFINITY);

//   // Install and start TWAI driver
//   // ESP_ERROR_CHECK(twai_driver_install(&g_config, &t_config, &f_config));
//   ESP_ERROR_CHECK(twai_driver_install(&twai_config->g_config, &twai_config->t_config, &twai_config->f_config));
//   ESP_LOGI(TAG_TWAI, "Driver installed");
//   // ESP_ERROR_CHECK(twai_start());
//   ESP_LOGI(TAG_TWAI, "Driver started");

//   xSemaphoreGive(rx_sem); // Start RX task
//   vTaskDelay(pdMS_TO_TICKS(100));
//   xSemaphoreTake(rx_sem, portMAX_DELAY); // Wait for RX task to complete

//   // Stop and uninstall TWAI driver
//   ESP_ERROR_CHECK(twai_stop());
//   ESP_LOGI(TAG_TWAI, "Driver stopped");
//   ESP_ERROR_CHECK(twai_driver_uninstall());
//   ESP_LOGI(TAG_TWAI, "Driver uninstalled");

//   // Cleanup
//   vSemaphoreDelete(rx_sem);

//   return NULL;
// }

esp_err_t esp_twai_client_stop(esp_twai_client_handle_t client)
{
  if (!client)
  {
    ESP_LOGE(TAG_TWAI, "Client was not initialized");
    return ESP_ERR_INVALID_ARG;
  }
  TWAI_API_LOCK(client);
  if (client->run)
  {
    /* A running client cannot be stopped from the MQTT task/event handler */
    TaskHandle_t running_task = xTaskGetCurrentTaskHandle();
    if (running_task == client->task_handle)
    {
      TWAI_API_UNLOCK(client);
      ESP_LOGE(TAG_TWAI, "Client cannot be stopped from MQTT task");
      return ESP_FAIL;
    }

    // Only send the disconnect message if the client is connected
    // if (client->state == TWAI_STATE_CONNECTED)
    // {
    //   if (send_disconnect_msg(client) != ESP_OK)
    //   {
    //     TWAI_API_UNLOCK(client);
    //     return ESP_FAIL;
    //   }
    // }

    client->run = false;
    client->state = TWAI_STATE_DISCONNECTED;
    TWAI_API_UNLOCK(client);
    xEventGroupWaitBits(client->status_bits, STOPPED_BIT, false, true, portMAX_DELAY);
    return ESP_OK;
  }
  else
  {
    ESP_LOGW(TAG_TWAI, "Client asked to stop, but was not started");
    TWAI_API_UNLOCK(client);
    return ESP_FAIL;
  }
}

/**
 * @brief Add user defined handler for NMEA parser
 *
 * @param twai handle of TWAI
 * @param event_handler user defined event handler
 * @param handler_args handler specific arguments
 * @return esp_err_t
 *  - ESP_OK: Success
 *  - ESP_ERR_NO_MEM: Cannot allocate memory for the handler
 *  - ESP_ERR_INVALIG_ARG: Invalid combination of event base and event id
 *  - Others: Fail
 */

esp_err_t esp_twai_client_register_event(esp_twai_client_handle_t client, esp_twai_event_id_t event, esp_event_handler_t event_handler, void *event_handler_arg)
{
  if (client == NULL)
  {
    return ESP_ERR_INVALID_ARG;
  }
  return esp_event_handler_register_with(client->config->event_loop_handle, TWAI_EVENTS, event, event_handler, event_handler_arg);

  ESP_LOGE(TAG_TWAI, "Registering event handler while event loop not available in IDF version %s", IDF_VER);
  return ESP_FAIL;
}

esp_err_t esp_twai_client_unregister_event(esp_twai_client_handle_t client, esp_twai_event_id_t event, esp_event_handler_t event_handler)
{
  if (client == NULL)
  {
    return ESP_ERR_INVALID_ARG;
  }

  ESP_LOGE(TAG_TWAI, "Unregistering event handler while event loop not available in IDF version %s", IDF_VER);
  return ESP_FAIL;
}
