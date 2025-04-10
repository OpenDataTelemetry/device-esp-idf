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

static const twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();
static const twai_timing_config_t t_config = TWAI_TIMING_CONFIG_1MBITS();
// Set TX queue length to 0 due to listen only mode
static const twai_general_config_t g_config = TWAI_GENERAL_CONFIG();

static SemaphoreHandle_t rx_sem;

/* --------- TWAI TASK BEGIN ---------*/
static void twai_configure(void *arg)
{
  esp_gps_t *esp_gps = (esp_gps_t *)arg;
  uart_event_t event;
  int len = 0;


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
  
  sendData(GPS_TASK_TAG, "AT+CGPSINFO=1\r\n");
  vTaskDelay(1000 / portTICK_PERIOD_MS);
  sendData(GPS_TASK_TAG, "AT+CGNSSINFO=1\r\n");
  vTaskDelay(1000 / portTICK_PERIOD_MS);
  sendData(GPS_TASK_TAG, "AT+CGNSSPORTSWITCH=0,1\r\n");
  vTaskDelay(1000 / portTICK_PERIOD_MS);
  sendData(GPS_TASK_TAG, "AT+CGNSSNMEA=1,1,1,1,1,1,0,0,0,0\\r\n");
  vTaskDelay(10000 / portTICK_PERIOD_MS);

  sendData(GPS_TASK_TAG, "AT+CGNSSTST=0\r\n");
  vTaskDelay(1000 / portTICK_PERIOD_MS);
  sendData(GPS_TASK_TAG, "AT+CGNSSTST=1\r\n");
  vTaskDelay(1000 / portTICK_PERIOD_MS);

  vTaskDelete(NULL);
}

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
/* --------- TWAI TASK END ---------*/

//========================================
/**
 * @brief Init TWAI
 *
 * @param twai_config Configuration of NMEA Parser
 * @return twai_handle_t handle of twai
 */
twai_handle_t twai_init(const twai_config_t *twai_config)
{
  esp_gps_t *esp_gps = calloc(1, sizeof(esp_gps_t));
  if (!esp_gps)
  {
    ESP_LOGE(TWAI_TAG, "calloc memory for esp_fps failed");
    goto err_gps;
  }
  esp_gps->buffer = calloc(1,TWAI_RUNTIME_BUFFER_SIZE);
  if (!esp_gps->buffer)
  {
    ESP_LOGE(TWAI_TAG, "calloc memory for runtime buffer failed");
    goto err_buffer;
  }
#if CONFIG_NMEA_STATEMENT_GSA
  esp_gps->all_statements |= (1 << STATEMENT_GSA);
#endif
#if CONFIG_NMEA_STATEMENT_GSV
  esp_gps->all_statements |= (1 << STATEMENT_GSV);
#endif
#if CONFIG_NMEA_STATEMENT_GGA
  esp_gps->all_statements |= (1 << STATEMENT_GGA);
#endif
#if CONFIG_NMEA_STATEMENT_RMC
  esp_gps->all_statements |= (1 << STATEMENT_RMC);
#endif
#if CONFIG_NMEA_STATEMENT_GLL
  esp_gps->all_statements |= (1 << STATEMENT_GLL);
#endif
#if CONFIG_NMEA_STATEMENT_VTG
  esp_gps->all_statements |= (1 << STATEMENT_VTG);
#endif
  /* Set attributes */
  esp_gps->uart_port = twai_config->uart.uart_port;
  esp_gps->all_statements &= 0xFE;
  /* Install UART friver */
  uart_config_t uart_config = {
      .baud_rate = twai_config->uart.baud_rate,
      .data_bits = twai_config->uart.data_bits,
      .parity = twai_config->uart.parity,
      .stop_bits = twai_config->uart.stop_bits,
      .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
      .source_clk = UART_SCLK_DEFAULT,
  };
  if (uart_driver_install(esp_gps->uart_port, CONFIG_NMEA_PARSER_RING_BUFFER_SIZE, 0,
    twai_config->uart.event_queue_size, &esp_gps->event_queue, 0) != ESP_OK)
  {
    ESP_LOGE(TWAI_TAG, "install uart driver failed");
    goto err_uart_install;
  }
  if (uart_param_config(esp_gps->uart_port, &uart_config) != ESP_OK)
  {
    ESP_LOGE(TWAI_TAG, "config uart parameter failed");
    goto err_uart_config;
  }
  if (uart_set_pin(esp_gps->uart_port, twai_config->uart.tx_pin, twai_config->uart.rx_pin,
                   UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE) != ESP_OK)
  {
    ESP_LOGE(TWAI_TAG, "config uart gpio failed");
    goto err_uart_config;
  }
  /* Set pattern interrupt, used to detect the end of a line */
  uart_enable_pattern_det_baud_intr(esp_gps->uart_port, '\n', 1, 9, 0, 0);
  /* Set pattern queue size */
  uart_pattern_queue_reset(esp_gps->uart_port, twai_config->uart.event_queue_size);
  uart_flush(esp_gps->uart_port);
  /* Create Event loop */
  esp_event_loop_args_t loop_args = {
      .queue_size = NMEA_EVENT_LOOP_QUEUE_SIZE,
      .task_name = NULL};
  if (esp_event_loop_create(&loop_args, &esp_gps->event_loop_hdl) != ESP_OK)
  {
    ESP_LOGE(TWAI_TAG, "create event loop faild");
    goto err_eloop;
  }

  //RC-EDIT
  xTaskCreate(twai_configure, "twai_configure", CONFIG_NMEA_PARSER_TASK_STACK_SIZE, esp_gps, CONFIG_NMEA_PARSER_TASK_PRIORITY, NULL);

  /* Create NMEA Parser task */
  BaseType_t err = xTaskCreate(
      twai_task_entry,
      "twai",
      CONFIG_NMEA_PARSER_TASK_STACK_SIZE,
      esp_gps,
      CONFIG_NMEA_PARSER_TASK_PRIORITY,
      &esp_gps->tsk_hdl);
  if (err != pdTRUE)
  {
    ESP_LOGE(TWAI_TAG, "create NMEA Parser task failed");
    goto err_task_create;
  }
  ESP_LOGI(TWAI_TAG, "NMEA Parser init OK");
  return esp_gps;
  /*Error Handling*/
err_task_create:
  esp_event_loop_delete(esp_gps->event_loop_hdl);
err_eloop:
err_uart_install:
  uart_driver_delete(esp_gps->uart_port);
err_uart_config:
err_buffer:
  free(esp_gps->buffer);
err_gps:
  free(esp_gps);
  return NULL;
}
//========================================
//========================================

//TWAI INIT
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