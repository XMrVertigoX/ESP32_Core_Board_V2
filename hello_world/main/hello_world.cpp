#include <stdio.h>

#include "esp_log.h"
#include "esp_spi_flash.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char tag[] = "hello_world.cpp";

static const uint32_t timeout = 10;
static esp_chip_info_t chip_info;

static void printHardwareDetails(void) {
    ESP_LOGI(tag, "Hello world! This is ESP32_Core_board_V2.");
    ESP_LOGI(tag, "Number of cores: %u", chip_info.cores);
    ESP_LOGI(tag, "Silicon revision: %u", chip_info.revision);
    ESP_LOGI(tag, "Memory: %u MB flash.", (spi_flash_get_chip_size() / (1024 * 1024)));
}

extern "C" void app_main() {
    esp_chip_info(&chip_info);
    printHardwareDetails();

    for (int i = timeout; i > 0; i--) {
        if (i == timeout) {
            ESP_LOGW(tag, "Restarting in %u second(s)...", i);
        } else {
            ESP_LOGD(tag, "Restarting in %u second(s)...", i);
        }

        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }

    ESP_LOGW(tag, "Restarting");

    esp_restart();
}
