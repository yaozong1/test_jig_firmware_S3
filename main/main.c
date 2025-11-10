// SPDX-License-Identifier: MIT
// Test Jig Firmware: DUT control (EN/IO0) + 8-channel voltage acquisition

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "driver/usb_serial_jtag.h"
#include "voltage_adc.h"  // K10-3U8 voltage module

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

// Simple command parser on USB-Serial-JTAG (!BOOT/!RUN/!RST/!VOLTAGE)
static void console_task(void *arg)
{
    uint8_t b;
    char cmd[32]; int len = 0; int in_cmd = 0;
    usj_write("JIG: READY (Control EN/IO0 + Voltage ADC)\r\n");
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
                        usj_write("JIG: BOOT OK (DUT in bootloader mode)\r\n");
                    } else if (!strcmp(cmd, "RUN")) {
                        usj_write("JIG: RUN start\r\n");
                        gpio_set_level(DUT_IO0_PIN, 1);
                        vTaskDelay(pdMS_TO_TICKS(2));
                        gpio_set_level(DUT_EN_PIN, 0);
                        vTaskDelay(pdMS_TO_TICKS(5));
                        gpio_set_level(DUT_EN_PIN, 1);
                        usj_write("JIG: RUN OK (DUT running)\r\n");
                    } else if (!strcmp(cmd, "RST")) {
                        usj_write("JIG: RST\r\n");
                        gpio_set_level(DUT_EN_PIN, 0);
                        vTaskDelay(pdMS_TO_TICKS(5));
                        gpio_set_level(DUT_EN_PIN, 1);
                    } else if (!strcmp(cmd, "VOLTAGE")) {
                        usj_write("JIG: Reading 8-channel voltage...\r\n");
                        uint16_t voltages[8] = {0};
                        bool ok = voltage_adc_read_all(voltages);
                        if (ok) {
                            voltage_adc_send_payload(voltages);
                            usj_write("JIG: Voltage read OK\r\n");
                        } else {
                            usj_write("JIG: Voltage read FAILED\r\n");
                        }
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

// Voltage reading task - continuously read and display voltage
static void voltage_task(void *arg)
{
    ESP_LOGI(TAG, ">>> Voltage task STARTING <<<");
    usj_write("\r\n>>> Voltage task started, reading every 2 seconds... <<<\r\n\r\n");
    vTaskDelay(pdMS_TO_TICKS(2000)); // Wait 2s for init
    
    int count = 0;
    while (1) {
        count++;
        char msg[64];
        snprintf(msg, sizeof(msg), "\r\n=== Reading Voltage [%d] ===\r\n", count);
        usj_write(msg);
        ESP_LOGI(TAG, "Voltage read attempt %d", count);
        
        uint16_t voltages[8] = {0};
        bool ok = voltage_adc_read_all(voltages);
        
        if (ok) {
            voltage_adc_send_payload(voltages);
            usj_write("Voltage read SUCCESS\r\n");
            ESP_LOGI(TAG, "Voltage read OK");
        } else {
            usj_write("Voltage read FAILED\r\n");
            ESP_LOGE(TAG, "Voltage read FAILED");
        }
        
        usj_write("======================\r\n\r\n");
        vTaskDelay(pdMS_TO_TICKS(2000)); // Read every 2 seconds
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "Test Jig Firmware - DUT Control + Voltage ADC");
    ESP_LOGI(TAG, "DUT Ctrl : EN=%d IO0=%d", DUT_EN_PIN, DUT_IO0_PIN);
    ESP_LOGI(TAG, "Voltage  : UART1 TX=%d RX=%d", VOLTAGE_UART_TX, VOLTAGE_UART_RX);
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

    // Initialize voltage ADC module
    ESP_LOGI(TAG, "Initializing voltage ADC...");
    voltage_adc_init();
    
    // Load default calibration parameters
    // 根据测试：AI1 读取 4801 mV (实际 4620 mV)，需要 -181 mV 校正
    ESP_LOGI(TAG, "Loading calibration parameters...");
    voltage_adc_load_default_calibration();
    
    ESP_LOGI(TAG, "Voltage ADC initialized with calibration");

    // Start console task
    ESP_LOGI(TAG, "Creating console task...");
    xTaskCreate(console_task, "console", 4096, NULL, 5, NULL);
    
    // Start voltage reading task (continuously read and display)
    ESP_LOGI(TAG, "Creating voltage task...");
    BaseType_t ret = xTaskCreate(voltage_task, "voltage", 4096, NULL, 4, NULL);
    if (ret == pdPASS) {
        ESP_LOGI(TAG, "Voltage task created successfully");
    } else {
        ESP_LOGE(TAG, "FAILED to create voltage task!");
    }

    ESP_LOGI(TAG, "Control running. Commands: !BOOT !RUN !RST !VOLTAGE");
    ESP_LOGI(TAG, "Voltage auto-reading enabled (every 2 seconds)");
    ESP_LOGI(TAG, "========================================");
}
