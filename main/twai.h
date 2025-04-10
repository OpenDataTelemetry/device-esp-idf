#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "esp_types.h"
#include "esp_event.h"
#include "esp_err.h"
#include "driver/twai.h"

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
      .clkout_divider = 0};


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
    twai_EVENT_ANY = -1,

  } twai_event_id_t;

  /**
   * @brief Declare of TWAI Event base
   *
   */
  ESP_EVENT_DECLARE_BASE(ESP_NMEA_EVENT);

  /**
   * @brief TWAI fix type
   *
   */
  typedef enum
  {
    TWAI_FIX_INVALID, /*!< Not fixed */
    TWAI_FIX_TWAI,    /*!< TWAI */
    TWAI_FIX_DTWAI,   /*!< Differential TWAI */
  } twai_fix_t;

  /**
   * @brief TWAI fix mode
   *
   */
  typedef enum
  {
    TWAI_MODE_INVALID = 1, /*!< Not fixed */
    TWAI_MODE_2D,          /*!< 2D TWAI */
    TWAI_MODE_3D           /*!< 3D TWAI */
  } twai_fix_mode_t;

  /**
   * @brief TWAI satellite information
   *
   */
  typedef struct
  {
    uint8_t num;       /*!< Satellite number */
    uint8_t elevation; /*!< Satellite elevation */
    uint16_t azimuth;  /*!< Satellite azimuth */
    uint8_t snr;       /*!< Satellite signal noise ratio */
  } twai_satellite_t;

  /**
   * @brief TWAI time
   *
   */
  typedef struct
  {
    uint8_t hour;      /*!< Hour */
    uint8_t minute;    /*!< Minute */
    uint8_t second;    /*!< Second */
    uint16_t thousand; /*!< Thousand */
  } twai_time_t;

  /**
   * @brief TWAI date
   *
   */
  typedef struct
  {
    uint8_t day;   /*!< Day (start from 1) */
    uint8_t month; /*!< Month (start from 1) */
    uint16_t year; /*!< Year (start from 2000) */
  } twai_date_t;

  /**
   * @brief NMEA Statement
   *
   */
  typedef enum
  {
    STATEMENT_UNKNOWN = 0, /*!< Unknown statement */
    STATEMENT_GGA,         /*!< GGA */
    STATEMENT_GSA,         /*!< GSA */
    STATEMENT_RMC,         /*!< RMC */
    STATEMENT_GSV,         /*!< GSV */
    STATEMENT_GLL,         /*!< GLL */
    STATEMENT_VTG          /*!< VTG */
  } twai_statement_t;

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
   * @brief Configuration of TWAI
   *
   */
  typedef struct
  {
    struct
    {
      uart_port_t uart_port; /*!< UART port number */
      uint32_t tx_pin;       /*!< UART Tx Pin number */
      uint32_t rx_pin;       /*!< UART Rx Pin number */
      uint32_t baud_rate;    /*!< UART baud rate */
    } twai;                  /*!< UART specific configuration */
  } twai_config_t;

  /**
   * @brief TWAI Handle
   *
   */
  typedef void *twai_handle_t;

/**
 * @brief Default configuration for TWAI
 *
 */
#define twai_CONFIG_DEFAULT()                  \
  {                                            \
    .uart = {                                  \
      .uart_port = UART_NUM_1,                 \
      .tx_pin = CONFIG_twai_UART_TXD,          \
      .rx_pin = CONFIG_twai_UART_RXD,          \
      .baud_rate = CONFIG_twai_UART_BAUD_RATE, \
    }                                          \
  }

  /**
   * @brief TWAI Event ID
   *
   */
  typedef enum
  {
    TWAI_UPDATE, /*!< TWAI information has been updated */
    TWAI_UNKNOWN /*!< Unknown statements detected */
  } twai_event_id_t;

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