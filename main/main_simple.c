// SPDX-License-Identifier: MIT
// Simplified Jig firmware: Only control EN/IO0, no UART bridge needed

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "driver/usb_serial_jtag.h"

static const char *TAG = "JIG";

// DUT control pins (driven by S3)
#ifndef DUT_EN_PIN
#define DUT_EN_PIN          4
#endif
#ifndef DUT_IO0_PIN
#define DUT_IO0_PIN         5
#endif

// Helper: write to USB-Serial-JTAG console
static void usj_write(const char *s)
{
    if (!s) return;
    size_t n = strlen(s);
    if (n) usb_serial_jtag_write_bytes((const uint8_t*)s, n, 0);
}

// Simple command parser on USB-Serial-JTAG (!BOOT/!RUN/!RST)
static void console_task(void *arg)
{
    uint8_t b;
    char cmd[32]; int len = 0; int in_cmd = 0;
    usj_write("JIG: READY (Control-only mode, CH340 directly connected to DUT)\r\n");
    while (1) {
        int r = usb_serial_jtag_read_bytes(&b, 1, 10);
        if (r > 0) {
            if (!in_cmd) {
                if (b == '!') { in_cmd = 1; len = 0; }
            } else {
                if (b == '\r' || b == '\n') {
                    cmd[len] = 0;
                    if (!strcmp(cmd, "BOOT")) {
                        usj_write("JIG: BOOT start\r\n");
                        // IO0 low, EN pulse
                        gpio_set_level(DUT_IO0_PIN, 0);
                        vTaskDelay(pdMS_TO_TICKS(2));
                        gpio_set_level(DUT_EN_PIN, 0);
                        vTaskDelay(pdMS_TO_TICKS(5));
                        gpio_set_level(DUT_EN_PIN, 1);
                        vTaskDelay(pdMS_TO_TICKS(12));
                        usj_write("JIG: BOOT OK (DUT in bootloader, use CH340 port to flash)\r\n");
                    } else if (!strcmp(cmd, "RUN")) {
                        usj_write("JIG: RUN start\r\n");
                        gpio_set_level(DUT_IO0_PIN, 1);
                        vTaskDelay(pdMS_TO_TICKS(2));
                        gpio_set_level(DUT_EN_PIN, 0);
                        vTaskDelay(pdMS_TO_TICKS(5));
                        gpio_set_level(DUT_EN_PIN, 1);
                        usj_write("JIG: RUN OK (DUT in application, read selftest from CH340 port)\r\n");
                    } else if (!strcmp(cmd, "RST")) {
                        usj_write("JIG: RST\r\n");
                        gpio_set_level(DUT_EN_PIN, 0);
                        vTaskDelay(pdMS_TO_TICKS(5));
                        gpio_set_level(DUT_EN_PIN, 1);
                    } else {
                        usj_write("JIG: UNKNOWN CMD\r\n");
                    }
                    in_cmd = 0; len = 0;
                } else if (len < (int)sizeof(cmd) - 1) {
                    if (b >= 'a' && b <= 'z') b = (uint8_t)(b - 'a' + 'A');
                    cmd[len++] = (char)b;
                }
            }
        } else {
            vTaskDelay(pdMS_TO_TICKS(1));
        }
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "Test Jig - Control Only (No UART bridge)");
    ESP_LOGI(TAG, "DUT Ctrl : EN=%d IO0=%d", DUT_EN_PIN, DUT_IO0_PIN);
    ESP_LOGI(TAG, "CH340 connected directly to DUT UART0");
    ESP_LOGI(TAG, "========================================");

    // Configure GPIOs
    gpio_config_t out_conf = {
        .pin_bit_mask = (1ULL << DUT_EN_PIN) | (1ULL << DUT_IO0_PIN),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ESP_ERROR_CHECK(gpio_config(&out_conf));
    gpio_set_level(DUT_EN_PIN, 1);
    gpio_set_level(DUT_IO0_PIN, 1);

    // USB-Serial-JTAG console
    usb_serial_jtag_driver_config_t usj_cfg = {
        .tx_buffer_size = 1024,
        .rx_buffer_size = 1024,
    };
    ESP_ERROR_CHECK(usb_serial_jtag_driver_install(&usj_cfg));

    // Start console task only
    xTaskCreate(console_task, "console", 4096, NULL, 5, NULL);

    ESP_LOGI(TAG, "Control console running. CH340 port is for direct DUT flash/monitor.");
}
