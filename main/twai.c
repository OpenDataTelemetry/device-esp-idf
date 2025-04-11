// /*UART ASYNC*/
// #include "freertos/FreeRTOS.h"
// #include "freertos/task.h"
// #include "esp_system.h"
// #include "esp_log.h"
// #include "driver/uart.h"
// #include "string.h"
// #include "driver/gpio.h"
// #include <stdio.h>
// #include <stdlib.h>
// /*CAN*/
// #include "freertos/queue.h"
// #include "freertos/semphr.h"
// #include "esp_err.h"
// #include "driver/twai.h"
// #include "twai.h"

#include <stdio.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "esp_err.h"
#include "esp_log.h"
#include "driver/twai.h"
#include "twai.h"


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

ESP_EVENT_DEFINE_BASE(ESP_TWAI_EVENT);

static const twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();
// static const twai_timing_config_t t_config = TWAI_TIMING_CONFIG_25KBITS();
static const twai_timing_config_t t_config = TWAI_TIMING_CONFIG_1MBITS();
// static const twai_timing_config_t t_config = TWAI_TIMING_CONFIG_1MBITS();
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

// static const twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();
// static const twai_timing_config_t t_config = TWAI_TIMING_CONFIG_1MBITS();
// // Set TX queue length to 0 due to listen only mode
// static const twai_general_config_t g_config = TWAI_GENERAL_CONFIG();

static SemaphoreHandle_t rx_sem;

/* --------- TWAI TASK INIT ---------*/
static void twai_receive_task(void *arg)
{
  xSemaphoreTake(rx_sem, portMAX_DELAY);
  printf("##################ENTERED TWAI TASK!");

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
/* --------- TWAI TASK END ---------*/

/**
 * @brief Init TWAI
 *
 * @param twai_config Configuration of NMEA Parser
 * @return twai_handle_t handle of twai
 */
twai_handle_t twai_init(const twai_config_t *twai_config)
{
  rx_sem = xSemaphoreCreateBinary();
  xTaskCreatePinnedToCore(twai_receive_task, "TWAI_rx", 4096, NULL, RX_TASK_PRIO, NULL, tskNO_AFFINITY);

  // Install and start TWAI driver
  ESP_ERROR_CHECK(twai_driver_install(&g_config, &t_config, &f_config));
  // ESP_ERROR_CHECK(twai_driver_install(&twai_config->g_config, &twai_config->t_config, &twai_config->f_config));
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

  // Cleanup
  vSemaphoreDelete(rx_sem);

  return NULL;
}

/**
 * @brief Add user defined handler for NMEA parser
 *
 * @param twai handle of NMEA parser
 * @param event_handler user defined event handler
 * @param handler_args handler specific arguments
 * @return esp_err_t
 *  - ESP_OK: Success
 *  - ESP_ERR_NO_MEM: Cannot allocate memory for the handler
 *  - ESP_ERR_INVALIG_ARG: Invalid combination of event base and event id
 *  - Others: Fail
 */
esp_err_t twai_register_event(twai_handle_t twai, twai_event_id_t event, esp_event_handler_t event_handler, void *event_handler_arg)
{
  return esp_event_handler_register_with(twai, ESP_TWAI_EVENT, event,
                                         event_handler, event_handler_arg);
}