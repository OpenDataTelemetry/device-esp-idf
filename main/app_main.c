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

/*UART ASYNC*/
static const int RX_BUF_SIZE = 2048;
#define TXD_PIN (GPIO_NUM_18)
#define RXD_PIN (GPIO_NUM_17)

/*CAN*/
/* --------------------- Definitions and static variables ------------------ */
// Example Configuration
#define NO_OF_ITERS 3
#define RX_TASK_PRIO 9
#define TX_GPIO_NUM CONFIG_EXAMPLE_TX_GPIO_NUM
#define RX_GPIO_NUM CONFIG_EXAMPLE_RX_GPIO_NUM
#define EXAMPLE_TAG "TWAI Listen Only"

#define ID_MASTER_STOP_CMD 0x0A0
#define ID_MASTER_START_CMD 0x0A1
#define ID_MASTER_PING 0x0A2
#define ID_SLAVE_STOP_RESP 0x0B0
#define ID_SLAVE_DATA 0x0B1
#define ID_SLAVE_PING_RESP 0x0B2

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

static SemaphoreHandle_t rx_sem;

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
    ESP_LOGI(EXAMPLE_TAG, "Received %u data bytes with value %" PRIu32, rx_msg.data_length_code, data);
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


// static void tx_task(void *arg)
// {
//   static const char *TX_TASK_TAG = "TX_TASK";
//   esp_log_level_set(TX_TASK_TAG, ESP_LOG_INFO);
//   while (1)
//   {
//     sendData(TX_TASK_TAG, "AT\r\n");
//     vTaskDelay(1000 / portTICK_PERIOD_MS);
    
//   }
// }

static void rx_task(void *arg)
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

void app_main(void)
{
  init();
  xTaskCreate(rx_task, "uart_rx_task", 2048 * 2, NULL, configMAX_PRIORITIES - 1, NULL);
  // xTaskCreate(tx_task, "uart_tx_task", 2048 * 2, NULL, configMAX_PRIORITIES - 2, NULL);

  
  // /*CAN*/
  rx_sem = xSemaphoreCreateBinary();
  xTaskCreatePinnedToCore(twai_receive_task, "TWAI_rx", 4096, NULL, RX_TASK_PRIO, NULL, tskNO_AFFINITY);

  // Install and start TWAI driver
  ESP_ERROR_CHECK(twai_driver_install(&g_config, &t_config, &f_config));
  ESP_LOGI(EXAMPLE_TAG, "Driver installed");
  ESP_ERROR_CHECK(twai_start());
  ESP_LOGI(EXAMPLE_TAG, "Driver started");

  xSemaphoreGive(rx_sem); // Start RX task
  vTaskDelay(pdMS_TO_TICKS(100));
  xSemaphoreTake(rx_sem, portMAX_DELAY); // Wait for RX task to complete

  // Stop and uninstall TWAI driver
  ESP_ERROR_CHECK(twai_stop());
  ESP_LOGI(EXAMPLE_TAG, "Driver stopped");
  ESP_ERROR_CHECK(twai_driver_uninstall());
  ESP_LOGI(EXAMPLE_TAG, "Driver uninstalled");

  // xTaskCreatePinnedToCore(twai_receive_task, "TWAI_rx", 4096, NULL, RX_TASK_PRIO, NULL, tskNO_AFFINITY);
  // Cleanup
  vSemaphoreDelete(rx_sem);

  send_gps_task();
}
