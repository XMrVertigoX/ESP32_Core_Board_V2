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
#include "esp_log.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "nvs_flash.h"

extern "C" {
#include "mqtt.h"
}

static const char TAG[]        = "MQTT";
static const int CONNECTED_BIT = BIT0;

static EventGroupHandle_t wifi_event_group;
static wifi_config_t wifi_config;
static mqtt_settings mqtt_config;

esp_err_t wifi_event_handler(void *ctx, system_event_t *event) {
    switch (event->event_id) {
        case SYSTEM_EVENT_STA_START: {
            esp_wifi_connect();
        } break;
        case SYSTEM_EVENT_STA_GOT_IP: {
            xEventGroupSetBits(wifi_event_group, CONNECTED_BIT);
            mqtt_start(&mqtt_config);
        } break;
        case SYSTEM_EVENT_STA_DISCONNECTED: {
            esp_wifi_connect();
            mqtt_stop();
            xEventGroupClearBits(wifi_event_group, CONNECTED_BIT);
        } break;
        default: {
            // do nothing by default
        } break;
    }

    return ESP_OK;
}

void wifi_init(void) {
    wifi_event_group       = xEventGroupCreate();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();

    memcpy(wifi_config.sta.ssid, CONFIG_WIFI_SSID, sizeof(CONFIG_WIFI_SSID));
    memcpy(wifi_config.sta.password, CONFIG_WIFI_PASSWORD, sizeof(CONFIG_WIFI_PASSWORD));

    tcpip_adapter_init();

    ESP_ERROR_CHECK(esp_event_loop_init(wifi_event_handler, NULL));
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
}

void mqtt_init() {
    memcpy(mqtt_config.host, "macbook", sizeof("macbook"));

#if defined(CONFIG_MQTT_SECURITY_ON)
    mqtt_config.port = 8883;
#else
    mqtt_config.port = 1883;
#endif

    mqtt_config.auto_reconnect = true;

    mqtt_config.connected_cb = [](mqtt_client *client, mqtt_event_data_t *event_data) {
        char topic[] = "/foo";
        char data[]  = "1";
        int data_len = sizeof(data);
        int qos      = 0;
        int retain   = 0;

        mqtt_publish(client, topic, data, data_len, qos, retain);
    };

    mqtt_config.data_cb = [](mqtt_client *client, mqtt_event_data_t *event_data) {
        ESP_LOGI(TAG, "Publish topic: %.*s", event_data->topic_length,
                 event_data->topic);
        ESP_LOGI(TAG, "Publish data: %.*s", event_data->data_length,
                 event_data->data);
    };
}

extern "C" void app_main() {
    nvs_flash_init();
    mqtt_init();
    wifi_init();
}
