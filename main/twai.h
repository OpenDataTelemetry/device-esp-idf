#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

#include "esp_types.h"
#include "esp_event.h"
#include "esp_err.h"
#include "driver/twai.h"

#define TWAI_MAX_SATELLITES_IN_USE (12)
#define TWAI_MAX_SATELLITES_IN_VIEW (16)

  /**
   * @brief *MQTT* event types.
   *
   * User event handler receives context data in `esp_mqtt_event_t` structure with
   *  - client - *MQTT* client handle
   *  - various other data depending on event type
   *
   */
  typedef enum twai_event_id_t
  {
    TWAI_EVENT_ANY = -1,
    TWAI_UPDATE, /*!< TWAI information has been updated */
    TWAI_UNKNOWN /*!< Unknown statements detected */

  } twai_event_id_t;

  /**
   * @brief Declare of TWAI Event base
   *
   */
  ESP_EVENT_DECLARE_BASE(ESP_TWAI_EVENT);

  /**
   * @brief TWAI object
   *
   */
  typedef struct
  {
    float latitude;                           /*!< Latitude (degrees) */
    float longitude;                          /*!< Longitude (degrees) */
    float altitude; /*!< Altitude (meters) */ /*!< Magnetic variation */
  } twai_t;

/**
 * @brief Default configuration for TWAI
 *
 */
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

  typedef struct
  {
    twai_filter_config_t f_config;
    twai_timing_config_t t_config;
    twai_general_config_t g_config;
  } twai_config_t;

#define TWAI_CONFIG_DEFAULT()          \
  {                                    \
      TWAI_FILTER_CONFIG_ACCEPT_ALL(), \
      TWAI_TIMING_CONFIG_1MBITS(),     \
      TWAI_GENERAL_CONFIG()}

  // /**
  //  * @brief TWAI Handle
  //  *
  //  */
  // typedef void *twai_handle_t;

  /**
   * @brief TWAI Event ID
   *
   */
  // typedef enum {
  //   TWAI_UPDATE, /*!< TWAI information has been updated */
  //   TWAI_UNKNOWN /*!< Unknown statements detected */
  // } twai_event_id_t;

  /**
   * @brief Init TWAI
   *
   * @param twai_config Configuration of TWAI
   * @return twai_handle_t handle of TWAI
   */
  twai_handle_t twai_init(const twai_config_t *twai_config);

  /**
   * @brief Deinit TWAI
   *
   * @param twai handle of TWAI
   * @return esp_err_t ESP_OK on success, ESP_FAIL on error
   */
  esp_err_t twai_deinit(twai_handle_t twai);

  /**
   * @brief Add user defined handler for TWAI
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
  esp_err_t twai_register_event(twai_handle_t twai, twai_event_id_t event, esp_event_handler_t event_handler, void *handler_args);

  /**
   * @brief Remove user defined handler for TWAI
   *
   * @param twai handle of TWAI
   * @param event_handler user defined event handler
   * @return esp_err_t
   *  - ESP_OK: Success
   *  - ESP_ERR_INVALIG_ARG: Invalid combination of event base and event id
   *  - Others: Fail
   */
  esp_err_t twai_remove_handler(twai_handle_t twai, esp_event_handler_t event_handler);

#ifdef __cplusplus
}
#endif