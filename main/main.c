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

// IGN test output pin - 100ms toggle for optocoupler test
#ifndef IGN_TEST_PIN
#define IGN_TEST_PIN        13  // S3的GPIO13用于IGN光耦测试输出
#endif

// Immobilizer test input pin - detect DUT's IM_OUT
#ifndef IM_IN_PIN
#define IM_IN_PIN           12  // S3的GPIO12用于检测DUT的IM_OUT光耦输出
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

// IGN test task - toggle IGN_TEST_PIN every 100ms for optocoupler test
static void ign_test_task(void *arg)
{
    ESP_LOGI(TAG, ">>> IGN test task STARTING (GPIO%d) <<<", IGN_TEST_PIN);
    usj_write("\r\n>>> IGN test task started, toggling every 100ms... <<<\r\n\r\n");
    
    int level = 0;
    while (1) {
        gpio_set_level(IGN_TEST_PIN, level);
        level = !level;
        vTaskDelay(pdMS_TO_TICKS(100));  // Toggle every 100ms
    }
}

// IM test task - detect IM_IN pin level changes from DUT's IM_OUT
bool im_test_result = false;
bool im_test_done = false;

static void im_test_task(void *arg)
{
    ESP_LOGI(TAG, ">>> IM test task STARTING (GPIO%d) <<<", IM_IN_PIN);
    usj_write("\r\n>>> IM test task started, detecting level changes... <<<\r\n\r\n");
    
    // 等待2秒让DUT启动
    vTaskDelay(pdMS_TO_TICKS(2000));
    
    // 持续循环测试,每5秒一次
    while (1) {
        int initial_level = gpio_get_level(IM_IN_PIN);
        ESP_LOGI(TAG, "IM test: starting new round, initial level=%d", initial_level);
        
        // 简化逻辑：只要检测到电平变化(翻转)就算通过
        // 不管初始是HIGH还是LOW,只需检测到: 当前电平 -> 反转 -> 再反转
        int last_level = initial_level;
        int toggle_count = 0;
        
        TickType_t start = xTaskGetTickCount();
        TickType_t timeout = pdMS_TO_TICKS(5000);  // 5秒超时
        
        while ((xTaskGetTickCount() - start) < timeout) {
            int level = gpio_get_level(IM_IN_PIN);
            
            if (level != last_level) {
                toggle_count++;
                ESP_LOGI(TAG, "IM test: toggle #%d, level %d->%d", toggle_count, last_level, level);
                last_level = level;
                
                // 检测到至少2次翻转(一个完整周期)就算通过
                if (toggle_count >= 2) {
                    ESP_LOGI(TAG, "IM test: detected %d toggles, PASS!", toggle_count);
                    im_test_result = true;
                    im_test_done = true;
                    usj_write("IM test: PASS\r\n");
                    break;  // 退出内层循环,5秒后重新测试
                }
            }
            
            vTaskDelay(pdMS_TO_TICKS(10));  // 10ms轮询间隔
        }
        
        // 超时则失败
        if (toggle_count < 2) {
            ESP_LOGW(TAG, "IM test: timeout, only detected %d toggles, FAIL", toggle_count);
            usj_write("IM test: FAIL (timeout)\r\n");
            im_test_result = false;
            im_test_done = true;
        }
        
        // 等待5秒后重新测试
        vTaskDelay(pdMS_TO_TICKS(5000));
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
        .pin_bit_mask = (1ULL << DUT_EN_PIN) | (1ULL << DUT_IO0_PIN) | (1ULL << IGN_TEST_PIN),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ESP_ERROR_CHECK(gpio_config(&out_conf));
    gpio_set_level(DUT_EN_PIN, 1);
    gpio_set_level(DUT_IO0_PIN, 1);
    gpio_set_level(IGN_TEST_PIN, 0);  // IGN test pin starts LOW

    // Configure IM_IN as input with pull-up
    gpio_config_t in_conf = {
        .pin_bit_mask = (1ULL << IM_IN_PIN),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ESP_ERROR_CHECK(gpio_config(&in_conf));

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

    // Start IGN test task (continuously toggle IGN_TEST_PIN every 100ms)
    ESP_LOGI(TAG, "Creating IGN test task...");
    ret = xTaskCreate(ign_test_task, "ign_test", 2048, NULL, 3, NULL);
    if (ret == pdPASS) {
        ESP_LOGI(TAG, "IGN test task created successfully");
    } else {
        ESP_LOGE(TAG, "FAILED to create IGN test task!");
    }

    // Start IM test task (detect IM_IN level changes from DUT)
    ESP_LOGI(TAG, "Creating IM test task...");
    ret = xTaskCreate(im_test_task, "im_test", 2048, NULL, 3, NULL);
    if (ret == pdPASS) {
        ESP_LOGI(TAG, "IM test task created successfully");
    } else {
        ESP_LOGE(TAG, "FAILED to create IM test task!");
    }

    ESP_LOGI(TAG, "Control running. Commands: !BOOT !RUN !RST !VOLTAGE");
    ESP_LOGI(TAG, "Voltage auto-reading enabled (every 2 seconds)");
    ESP_LOGI(TAG, "IGN test auto-toggling enabled (every 100ms on GPIO%d)", IGN_TEST_PIN);
    ESP_LOGI(TAG, "========================================");
}
