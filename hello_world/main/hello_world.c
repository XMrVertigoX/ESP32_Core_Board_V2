#include <stdio.h>

#include "esp_log.h"
#include "esp_spi_flash.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const uint32_t timeout = 10;
static esp_chip_info_t chip_info;

static void printHardwareDetails(void) {
    ESP_LOGI(__func__, "Hello world! This is ESP32_Core_board_V2.");
    ESP_LOGI(__func__, "Number of cores: %u", chip_info.cores);
    ESP_LOGI(__func__, "Silicon revision: %u", chip_info.revision);
    // TODO: Print if flash is embedded or external
    ESP_LOGI(__func__, "Memory: %u MB flash.", (spi_flash_get_chip_size() / (1024 * 1024)));
    // TODO: Print some feature information (Wifi/BT/BLE)
}

void app_main() {
    esp_chip_info(&chip_info);
    printHardwareDetails();

    for (int i = timeout; i > 0; i--) {
        if (i == timeout) {
            ESP_LOGW(__func__, "Restarting in %u seconds...", i);
        } else {
            ESP_LOGD(__func__, "Restarting in %u seconds...", i);
        }

        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }

    ESP_LOGW(__func__, "Restarting");

    esp_restart();
}
