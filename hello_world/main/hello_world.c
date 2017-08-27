#include <stdio.h>

#include "esp_spi_flash.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

void app_main() {
    printf("Hello world!\n");

    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);

    printf("This is ESP32 chip with %d CPU cores with WiFi", chip_info.cores);
    if ((chip_info.features & CHIP_FEATURE_BT)) printf("/BT");
    if ((chip_info.features & CHIP_FEATURE_BLE)) printf("/BLE");
    printf("\n");

    printf("Silicon revision: %d\n", chip_info.revision);

    printf("Memory: %d MB", (spi_flash_get_chip_size() / (1024 * 1024)));
    if (chip_info.features & CHIP_FEATURE_EMB_FLASH) {
        printf(" embedded ");
    } else {
        printf(" external ");
    }
    printf("flash\n");

    for (int i = 10; i > 0; i--) {
        printf("Restarting in %02d seconds...\n", i);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }

    printf("Restarting now!\n");

    fflush(stdout);

    esp_restart();
}
