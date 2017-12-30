#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

#include "lwip/dns.h"
#include "lwip/netdb.h"
#include "lwip/sockets.h"

#include "esp_event_loop.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "mqtt.h"
#include "nvs_flash.h"

#include "esp_log.h"

EventGroupHandle_t wifi_event_group;

const char *MQTT_TAG           = "MQTT_SAMPLE";
const static int CONNECTED_BIT = BIT0;

void connected_cb(void *self, void *params) {
    mqtt_client *client = (mqtt_client *)self;

    char *topic = "/foo";
    char *data  = "0";
    int len     = sizeof(data);
    int qos     = 0;
    int retain  = 0;

    mqtt_publish(client, topic, data, sizeof(data), qos, retain);
}

void disconnected_cb(void *self, void *params) {}

void reconnect_cb(void *self, void *params) {}

void subscribe_cb(void *self, void *params) {}

void publish_cb(void *self, void *params) {}

void data_cb(void *self, void *params) {
    mqtt_client *client           = (mqtt_client *)self;
    mqtt_event_data_t *event_data = (mqtt_event_data_t *)params;

    ESP_LOGI(MQTT_TAG, "Publish topic: %.*s", event_data->topic_length, event_data->topic);
    ESP_LOGI(MQTT_TAG, "Publish data: %.*s", event_data->data_length, event_data->data);
}

mqtt_settings settings = {
    .host = "192.168.1.114",
#if defined(CONFIG_MQTT_SECURITY_ON)
    .port = 8883,
#else
    .port = 1883,
#endif
    .client_id     = "mqtt_client_id",
    .username      = "user",
    .password      = "pass",
    .clean_session = 0,
    .keepalive     = 120,
    .lwt_topic     = "/foo",
    .lwt_msg       = "offline",
    .lwt_qos       = 0,
    .lwt_retain    = 0,
    .connected_cb  = connected_cb,
    // .disconnected_cb = disconnected_cb,
    // .subscribe_cb    = subscribe_cb,
    // .publish_cb      = publish_cb,
    .data_cb        = data_cb,
    .auto_reconnect = true};

esp_err_t wifi_event_handler(void *ctx, system_event_t *event) {
    switch (event->event_id) {
        case SYSTEM_EVENT_STA_START: {
            esp_wifi_connect();
        } break;
        case SYSTEM_EVENT_STA_GOT_IP: {
            xEventGroupSetBits(wifi_event_group, CONNECTED_BIT);
            mqtt_start(&settings);
        } break;
        case SYSTEM_EVENT_STA_DISCONNECTED: {
            esp_wifi_connect();
            mqtt_stop();
            xEventGroupClearBits(wifi_event_group, CONNECTED_BIT);
        } break;
        default: {
        } break;
    }

    return ESP_OK;
}

void wifi_conn_init(void) {
    tcpip_adapter_init();
    wifi_event_group = xEventGroupCreate();
    ESP_ERROR_CHECK(esp_event_loop_init(wifi_event_handler, NULL));
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    wifi_config_t wifi_config = {
        .sta =
            {
                .ssid = CONFIG_WIFI_SSID, .password = CONFIG_WIFI_PASSWORD,
            },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
}

void app_main() {
    nvs_flash_init();
    wifi_conn_init();
}
