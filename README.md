# DEVICE-ESP-IDF

| Tested Targets | ESP32 | ESP32-C6 | ESP32-S3 |
| -------------- | ----- | -------- | -------- |

## ESP32
* esp32
* esp32c6
* esp32s3

## Peripherals
UART_0
UART_1
UART_2



Always "esp_mqtt_dispatch_event" after "client->event.event_id = ATTRIBUTION"

esp_mqtt_client_register_event

- esp_event_handler_register_with



esp_mqtt_client_start(client);

  - xTaskCreate(esp_mqtt_task, "mqtt_task", client->config->task_stack, client, client->config->task_prio, &client->task_handle) != pdTRUE



esp_mqtt_task

  - xEventGroupClearBits(client->status_bits, STOPPED_BIT);
    
  - while (client->run)
    STATE_MACHINE: INIT -> CONNECTED -> INIT

    - run_event_loop(client)
    - switch (state)

      - case MQTT_STATE_DISCONNECTED:
       // When run is false. Exit While Loop
      - case MQTT_STATE_INIT:
        - xEventGroupClearBits(client->status_bits, RECONNECT_BIT | DISCONNECT_BIT);
        - client->event.event_id = MQTT_EVENT_BEFORE_CONNECT;
        - esp_mqtt_dispatch_event_with_msgid
        - esp_mqtt_dispatch_event
        - esp_mqtt_client_create_transport(client) != ESP_OK
        - esp_mqtt_connect(client, client->config->network_timeout_ms)
        - client->event.event_id = MQTT_EVENT_CONNECTED;
        - client->state = MQTT_STATE_CONNECTED;
        - esp_mqtt_dispatch_event_with_msgid(client);


      - case MQTT_STATE_CONNECTED:
        - xEventGroupWaitBits(client->status_bits, DISCONNECT_BIT, true, true, 0)
        - client->state = MQTT_STATE_INIT;

      - case MQTT_STATE_WAIT_RECONNECT:
        - xEventGroupClearBits(client->status_bits, RECONNECT_BIT);
        - client->state = MQTT_STATE_INIT;



esp_mqtt_dispatch_event
  - esp_event_post_to(client->config->event_loop_handle, MQTT_EVENTS, client->event.event_id, &client->event, sizeof(client->event), portMAX_DELAY);
  - ret = esp_event_loop_run(client->config->event_loop_handle, 0);











```text
    ----------   ----------   --------------
   |  Master  | |  Slave   | | Listen Only  |
   |          | |          | |              |
   | TX    RX | | TX    RX | | TX    RX     |
    ----------   ----------   --------------
     |      |     |      |     |      |
     |      |     |      |     |      |
    ----------   ----------   ----------
   | D      R | | D      R | | D      R |
   |          | |          | |          |
   |  VP230   | |  VP230   | |  VP230   |
   |          | |          | |          |
   | H      L | | H      L | | H      L |
    ----------   ----------   ----------
     |      |     |      |     |      |
     |      |     |      |     |      |
  |--x------|-----x------|-----x------|--| H
            |            |            |
  |---------x------------x------------x--| L

```




















# NMEA Parser Example

(See the README.md file in the upper level 'examples' directory for more information about examples.)

## Overview

This example will show how to parse NMEA-0183 data streams output from GPS/BDS/GLONASS modules based on ESP UART Event driver and ESP event loop library.
For the convenience of the presentation, this example will only parse the following basic statements:
* GGA
* GSA
* GSV
* RMC
* GLL
* VTG

See [Limitation for multiple navigation system](#Limitation) for more information about this example.

Usually, modules will also output some vendor specific statements which common nmea library can not cover. In this example, the NMEA Parser will propagate all unknown statements to the user, where a custom handler can parse information from it.

## How to use example

### Hardware Required

To run this example, you need an ESP32, ESP32-S or ESP32-C series dev board (e.g. ESP32-WROVER Kit). For test purpose, you also need a GPS module. Here we take the [ATGM332D-5N](http://www.icofchina.com/pro/mokuai/2016-08-01/5.html) as an example to show how to parse the NMEA statements and output common information such as UTC time, latitude, longitude, altitude, speed and so on.

#### Pin Assignment:

**Note:** The following pin assignments are used by default which can be changed in `nmea_parser_config_t` structure.

| ESP                        | GPS             |
| -------------------------- | --------------- |
| UART-RX (GPIO5 by default) | GPS-TX          |
| GND                        | GND             |
| 5V                         | VCC             |

**Note:** UART TX pin is not necessary if you only use UART to receive data.


### Configure the project

Open the project configuration menu (`idf.py menuconfig`). Then go into `Example Configuration` menu.

- Set the size of ring buffer used by uart driver in `NMEA Parser Ring Buffer Size` option.
- Set the stack size of the NMEA Parser task in `NMEA Parser Task Stack Size` option.
- Set the priority of the NMEA Parser task in `NMEA Parser Task Priority` option.
- In the `NMEA Statement support` submenu, you can choose the type of statements that you want to parse. **Note:** you should choose at least one statement to parse.

### Build and Flash

Run `idf.py -p PORT flash monitor` to build and flash the project..

(To exit the serial monitor, type ``Ctrl-]``.)

See the [Getting Started Guide](https://docs.espressif.com/projects/esp-idf/en/latest/get-started/index.html) for full steps to configure and use ESP-IDF to build projects.

## Example Output

```bash
I (0) cpu_start: Starting scheduler on APP CPU.
I (317) uart: queue free spaces: 16
I (317) nmea_parser: NMEA Parser init OK
I (1067) gps_demo: 2018/12/4 13:59:34 =>
						latitude   = 31.20177°N
						longitude = 121.57933°E
						altitude   = 17.30m
						speed      = 0.370400m/s
W (1177) gps_demo: Unknown statement:$GPTXT,01,01,01,ANTENNA OK*35
I (2067) gps_demo: 2018/12/4 13:59:35 =>
						latitude   = 31.20177°N
						longitude  = 121.57933°E
						altitude   = 17.30m
						speed      = 0.000000m/s
W (2177) gps_demo: Unknown statement:$GPTXT,01,01,01,ANTENNA OK*35
I (3067) gps_demo: 2018/12/4 13:59:36 =>
						latitude   = 31.20178°N
						longitude  = 121.57933°E
						altitude   = 17.30m
						speed      = 0.000000m/s
W (3177) gps_demo: Unknown statement:$GPTXT,01,01,01,ANTENNA OK*35
I (4067) gps_demo: 2018/12/4 13:59:37 =>
						latitude   = 31.20178°N
						longitude  = 121.57933°E
						altitude   = 17.30m
						speed      = 0.000000m/s
W (4177) gps_demo: Unknown statement:$GPTXT,01,01,01,ANTENNA OK*35
I (5067) gps_demo: 2018/12/4 13:59:38 =>
						latitude   = 31.20178°N
						longitude  = 121.57933°E
						altitude   = 17.30m
						speed      = 0.685240m/s
W (5177) gps_demo: Unknown statement:$GPTXT,01,01,01,ANTENNA OK*35
```
As shown above, the ESP board finally got the information after parsed the NMEA0183 format statements. But as we didn't add `GPTXT` type statement in the library (that means it is UNKNOWN to NMEA Parser library), so it was propagated to user without any process.

## Troubleshooting

1. I cannot receive any statements from GPS although I have checked all the pin connections.
   * Test your GPS via other terminal (e.g. minicom, putty) to check the right communication parameters (e.g. baudrate supported by GPS).

## Limitation
If the GPS module supports multiple satellite navigation system (e.g. GPS, BDS), then the satellite ids and descriptions may be delivered in different statements (e.g. GPGSV, BDGSV, GPGSA, BDGSA), depend on the version of NMEA protocol used by the GPS module. This example currently can only record id and description of satellites from one navigation system.
However, for other statements, this example can parse them correctly whatever the navigation system is.

### Steps to skip the limitation
1. Uncheck the `GSA` and `GSV` statements in menuconfig
2. In the `gps_event_handler` will get a signal called `GPS_UNKNOWN`, and the unknown statement itself (It's a deep copy of the original statement).
3. Manually parse the unknown statements and get the satellites' descriptions.

(For any technical queries, please open an [issue](https://github.com/espressif/esp-idf/issues) on GitHub. We will get back to you as soon as possible.)




# .h
## mqtt_client.h
```h
typedef struct esp_mqtt_client *esp_mqtt_client_handle_t;

typedef enum esp_mqtt_event_id_t {
    MQTT_EVENT_ANY = -1,
    MQTT_EVENT_ERROR = 0,
    MQTT_EVENT_CONNECTED,                 
    MQTT_EVENT_DISCONNECTED, 
    MQTT_EVENT_SUBSCRIBED,                         
    MQTT_EVENT_UNSUBSCRIBED, msg_id */
    MQTT_EVENT_PUBLISHED,    
    MQTT_EVENT_DATA,                                 
    MQTT_EVENT_BEFORE_CONNECT, 
    MQTT_EVENT_DELETED,                      
    MQTT_USER_EVENT,                           
} esp_mqtt_event_id_t;


typedef enum esp_mqtt_error_type_t {
    MQTT_ERROR_TYPE_NONE = 0,
    MQTT_ERROR_TYPE_TCP_TRANSPORT,
    MQTT_ERROR_TYPE_CONNECTION_REFUSED,
    MQTT_ERROR_TYPE_SUBSCRIBE_FAILED
} esp_mqtt_error_type_t;


typedef struct esp_mqtt_event_t {
    esp_mqtt_event_id_t event_id;    
    esp_mqtt_client_handle_t client; 
    char *data;                      
    int data_len;                   
    int total_data_len;
    int current_data_offset; 
    char *topic;             
    int topic_len;
    int msg_id;   
    int session_present; 
    esp_mqtt_error_codes_t
    *error_handle; 
    bool retain; 
    int qos;    
    bool dup;   
    esp_mqtt_protocol_ver_t protocol_ver;
} esp_mqtt_event_t;

typedef esp_mqtt_event_t *esp_mqtt_event_handle_t;

typedef struct esp_mqtt_client_config_t {
    struct broker_t {
        struct address_t {
            const char *uri;
            const char *hostname;
            esp_mqtt_transport_t transport; 
            const char *path;               
            uint32_t port;                  
        } address;
        struct verification_t {
            bool use_global_ca_store;
            esp_err_t (*crt_bundle_attach)(void *conf); 
            const char *certificate; 
            size_t certificate_len;
            const struct psk_key_hint *psk_hint_key;
                                             
            bool skip_cert_common_name_check;
            const char **alpn_protos;       
            const char *common_name;       
        } verification;
    } broker;
    struct credentials_t {
        const char *username;   
        const char *client_id;
        bool set_null_client_id; 
        struct authentication_t {
            const char *password;   
            const char *certificate; 
            size_t certificate_len;  
            const char *key;    
            size_t key_len;
            const char *key_password;
            int key_password_len;   
            bool use_secure_element;
            void *ds_data; 
        } authentication;
    } credentials;
    struct session_t {
        struct last_will_t {
            const char *topic;
            const char *msg; 
            int msg_len;
            int qos; 
            int retain;
        } last_will;
        bool disable_clean_session;
        int keepalive;             
        bool disable_keepalive;
        esp_mqtt_protocol_ver_t protocol_ver; 
        int message_retransmit_timeout; 
    } session;
    struct network_t {
        int reconnect_timeout_ms; 
        int timeout_ms;
        int refresh_connection_after_ms; 
        bool disable_auto_reconnect;     
        esp_transport_handle_t transport;
        struct ifreq * if_name;
    struct task_t {
        int priority;
        int stack_size; 
    } task;
    struct buffer_t {
        int size;    
        int out_size;
    } buffer;
    struct outbox_config_t {
        uint64_t limit;
    } outbox;
} esp_mqtt_client_config_t;


typedef struct topic_t {
    const char *filter; 
    int qos;
} esp_mqtt_topic_t;

esp_mqtt_client_handle_t esp_mqtt_client_init(const esp_mqtt_client_config_t *config);
esp_err_t esp_mqtt_client_set_uri(esp_mqtt_client_handle_t client, const char *uri);
esp_err_t esp_mqtt_client_start(esp_mqtt_client_handle_t client);
esp_err_t esp_mqtt_client_reconnect(esp_mqtt_client_handle_t client);
esp_err_t esp_mqtt_client_disconnect(esp_mqtt_client_handle_t client);
esp_err_t esp_mqtt_client_stop(esp_mqtt_client_handle_t client);

int esp_mqtt_client_subscribe_single(esp_mqtt_client_handle_t client, const char *topic, int qos);
int esp_mqtt_client_subscribe_multiple(esp_mqtt_client_handle_t client, const esp_mqtt_topic_t *topic_list, int size);
int esp_mqtt_client_unsubscribe(esp_mqtt_client_handle_t client, const char *topic);
int esp_mqtt_client_publish(esp_mqtt_client_handle_t client, const char *topic, const char *data, int len, int qos, int retain);
int esp_mqtt_client_enqueue(esp_mqtt_client_handle_t client, const char *topic, const char *data, int len, int qos, int retain, bool store);

esp_err_t esp_mqtt_client_destroy(esp_mqtt_client_handle_t client);
esp_err_t esp_mqtt_set_config(esp_mqtt_client_handle_t client, const esp_mqtt_client_config_t *config);
esp_err_t esp_mqtt_client_register_event(esp_mqtt_client_handle_t client, esp_mqtt_event_id_t event, esp_event_handler_t event_handler, void *event_handler_arg);

esp_err_t esp_mqtt_client_unregister_event(esp_mqtt_client_handle_t client, esp_mqtt_event_id_t event, esp_event_handler_t event_handler);

int esp_mqtt_client_get_outbox_size(esp_mqtt_client_handle_t client);

esp_err_t esp_mqtt_dispatch_custom_event(esp_mqtt_client_handle_t client, esp_mqtt_event_t *event);
```

## mqtt_client_priv.h
typedef struct mqtt_state {}mqtt_state_t
typedef struct {}mqtt_config_storage_t
typedef enum {} mqtt_client_state_t;

```h
typedef struct mqtt_state {
    uint8_t *in_buffer;
    int in_buffer_length;
    size_t message_length;
    size_t in_buffer_read_len;
    mqtt_connection_t connection;
    uint16_t pending_msg_id;
    int pending_msg_type;
    int pending_publish_qos;
} mqtt_state_t;

typedef struct {
    esp_event_loop_handle_t event_loop_handle;
    int task_stack;
    int task_prio;
    char *uri;
    char *host;
    char *path;
    char *scheme;
    int port;
    bool auto_reconnect;
    int network_timeout_ms;
    int refresh_connection_after_ms;
    int reconnect_timeout_ms;
    char **alpn_protos;
    int num_alpn_protos;
    char *clientkey_password;
    int clientkey_password_len;
    bool use_global_ca_store;
    esp_err_t ((*crt_bundle_attach)(void *conf));
    const char *cacert_buf;
    size_t cacert_bytes;
    const char *clientcert_buf;
    size_t clientcert_bytes;
    const char *clientkey_buf;
    size_t clientkey_bytes;
    const struct psk_key_hint *psk_hint_key;
    bool skip_cert_common_name_check;
    const char *common_name;
    bool use_secure_element;
    void *ds_data;
    int message_retransmit_timeout;
    uint64_t outbox_limit;
    esp_transport_handle_t transport;
    struct ifreq * if_name;
} mqtt_config_storage_t;

typedef enum {
    MQTT_STATE_INIT = 0,
    MQTT_STATE_DISCONNECTED,
    MQTT_STATE_CONNECTED,
    MQTT_STATE_WAIT_RECONNECT,
} mqtt_client_state_t;

struct esp_mqtt_client {
    esp_transport_list_handle_t transport_list;
    esp_transport_handle_t transport;
    mqtt_config_storage_t *config;
    mqtt_state_t  mqtt_state;
    _Atomic mqtt_client_state_t state;
    uint64_t refresh_connection_tick;
    int64_t keepalive_tick;
    uint64_t reconnect_tick;
    int wait_timeout_ms;
    int auto_reconnect;
    esp_mqtt_event_t event;
    bool run;
    bool wait_for_ping_resp;
    outbox_handle_t outbox;
    EventGroupHandle_t status_bits;
    SemaphoreHandle_t  api_lock;
    TaskHandle_t       task_handle;
};

bool esp_mqtt_set_if_config(char const *const new_config, char **old_config);
void esp_mqtt_destroy_config(esp_mqtt_client_handle_t client);

```