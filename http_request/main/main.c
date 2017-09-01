#include <string.h>

#include "esp_event_loop.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include "lwip/dns.h"
#include "lwip/err.h"
#include "lwip/netdb.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "nvs_flash.h"

#define __INFINITE_LOOP for (;;)

#define WEB_SERVER "example.com"
#define WEB_PORT "80"

/* FreeRTOS event group to signal when we are connected & ready to make a request */
static EventGroupHandle_t wifi_event_group;

static const char TAG[] = "http_request_example";

static const char REQUEST[] =
    "GET / HTTP/1.1\r\n"
    "\r\n";

static esp_err_t wifiEventHandler(void *ctx, system_event_t *event) {
    switch (event->event_id) {
        case SYSTEM_EVENT_STA_START: {
            esp_wifi_connect();
        } break;

        case SYSTEM_EVENT_STA_GOT_IP: {
            xEventGroupSetBits(wifi_event_group, BIT0);
        } break;

        case SYSTEM_EVENT_STA_DISCONNECTED: {
            /* This is a workaround as ESP32 WiFi libs don't currently auto-reassociate. */
            xEventGroupClearBits(wifi_event_group, BIT0);
            esp_wifi_connect();
        } break;

        default: break;
    }

    return ESP_OK;
}

static esp_err_t initialise_wifi(void) {
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    wifi_sta_config_t sta = {.ssid = EXAMPLE_WIFI_SSID, .password = EXAMPLE_WIFI_PASS};
    wifi_config_t wifi_config = {.sta = sta};

    wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_event_loop_init(wifiEventHandler, NULL));
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    return (ESP_OK);
}

static void http_get_task(void *pvParameters) {
    struct addrinfo *res;
    struct in_addr *addr;
    int socket;
    int response;
    int error;
    char recv_buf[64];

    struct addrinfo hints = {
        .ai_family   = AF_INET,
        .ai_socktype = SOCK_STREAM,
    };

    __INFINITE_LOOP {
        /* Wait for the callback to set BIT0 in the event group. */
        xEventGroupWaitBits(wifi_event_group, BIT0, pdFALSE, pdTRUE, portMAX_DELAY);

        error = getaddrinfo(WEB_SERVER, WEB_PORT, &hints, &res);

        if (error != 0 || res == NULL) {
            ESP_LOGE(__func__, "DNS lookup failed (%d)", error);
            continue;
        }

        /* Code to print the resolved IP. Note: inet_ntoa is non-reentrant, look at for "real" code */
        addr = &((struct sockaddr_in *)res->ai_addr)->sin_addr;
        ESP_LOGI(__func__, "DNS lookup succeeded. IP=%s", inet_ntoa(*addr));

        socket = socket(res->ai_family, res->ai_socktype, 0);

        if (socket < 0) {
            ESP_LOGE(TAG, "... Failed to allocate socket.");
            freeaddrinfo(res);
            continue;
        }

        ESP_LOGI(TAG, "... allocated socket");

        error = connect(socket, res->ai_addr, res->ai_addrlen);

        if (error != 0) {
            ESP_LOGE(TAG, "Socket connect failed (%d)", errno);
            close(socket);
            freeaddrinfo(res);
            continue;
        }

        freeaddrinfo(res);

        ESP_LOGI(TAG, "... connected");

        error = write(socket, REQUEST, strlen(REQUEST));

        if (error < 0) {
            ESP_LOGE(TAG, "... socket send failed");
            close(socket);
            continue;
        }

        ESP_LOGI(TAG, "... socket send success");

        do {
            bzero(recv_buf, sizeof(recv_buf));
            response = read(socket, recv_buf, sizeof(recv_buf) - 1);
            for (int i = 0; i < response; i++) {
                putchar(recv_buf[i]);
            }
        } while (response > 0);

        ESP_LOGI(TAG, "... done reading from socket");

        close(socket);

        for (int countdown = 10; countdown >= 0; countdown--) {
            ESP_LOGI(TAG, "%d... ", countdown);
            vTaskDelay(1000 / portTICK_PERIOD_MS);
        }

        ESP_LOGI(TAG, "Starting again!");
    }
}

void app_main() {
    ESP_ERROR_CHECK(nvs_flash_init());
    tcpip_adapter_init();

    initialise_wifi();

    xTaskCreate(&http_get_task, "http_get_task", 4096, NULL, 5, NULL);
}
