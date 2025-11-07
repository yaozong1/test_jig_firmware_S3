// 最小测试固件 - 只测试 USB-Serial-JTAG 输出
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

static const char *TAG = "TEST";

void app_main(void)
{
    printf("\n\n");
    printf("================================\n");
    printf("MINIMAL TEST FIRMWARE\n");
    printf("Testing USB-Serial-JTAG output\n");
    printf("================================\n");
    printf("\n");
    
    ESP_LOGI(TAG, "ESP32-S3 is ALIVE!");
    ESP_LOGI(TAG, "This is a minimal test");
    
    int count = 0;
    while (1) {
        count++;
        printf(">>> Hello from ESP32-S3! Count=%d <<<\n", count);
        ESP_LOGI(TAG, "Loop iteration: %d", count);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
