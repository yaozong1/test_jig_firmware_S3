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
#include "rs485_tx.h"
#include "can_tx.h"

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

// Global variables for IM test results (used by voltage_adc.c)
bool im_test_result = false;
bool im_test_done = false;

// Helper: write to USB-Serial-JTAG console
static void usj_write(const char *s)
{
    if (!s) return;
    size_t n = strlen(s);
    if (n) usb_serial_jtag_write_bytes((const uint8_t*)s, n, 0);
}

// IM test function - execute once when requested
static bool run_im_test(void)
{
    ESP_LOGI(TAG, ">>> Starting IM test (on-demand) <<<");
    usj_write("\r\n[IM TEST] Starting...\r\n");
    
    // Reset test state
    im_test_done = false;
    im_test_result = false;
    
    int initial_level = gpio_get_level(IM_IN_PIN);
    ESP_LOGI(TAG, "IM test: initial level=%d", initial_level);
    
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
                usj_write("[IM TEST] PASS (detected toggles)\r\n");
                im_test_result = true;
                im_test_done = true;
                return true;
            }
        }
        
        vTaskDelay(pdMS_TO_TICKS(10));  // 10ms轮询间隔
    }
    
    // 超时则失败
    ESP_LOGW(TAG, "IM test: timeout, only detected %d toggles, FAIL", toggle_count);
    usj_write("[IM TEST] FAIL (timeout, no toggles detected)\r\n");
    im_test_result = false;
    im_test_done = true;
    return false;
}

// Simple command parser on USB-Serial-JTAG (!BOOT/!RUN/!RST/!VOLTAGE)
static void console_task(void *arg)
{
    uint8_t b;
    char cmd[32]; int len = 0; int in_cmd = 0;
    usj_write("JIG: READY (Control EN/IO0 + Voltage ADC)\r\n");
    usj_write("JIG: Waiting for !GUI_REQUEST_DATA...\r\n");
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
                        // Robust BOOT sequence for download mode:
                        // 1. Pull IO0 low immediately and hold 200ms
                        gpio_set_level(DUT_IO0_PIN, 0);
                        vTaskDelay(pdMS_TO_TICKS(200));  // IO0 stays low for 200ms
                        // 2. Pull EN low for 300ms (reset pulse)
                        gpio_set_level(DUT_EN_PIN, 0);
                        vTaskDelay(pdMS_TO_TICKS(300));  // EN low for 300ms
                        gpio_set_level(DUT_EN_PIN, 1);   // Release EN (goes high)
                        // 3. Wait another 300ms, then pull IO0 high
                        vTaskDelay(pdMS_TO_TICKS(300));  // Keep IO0 low for 300ms after EN high
                        gpio_set_level(DUT_IO0_PIN, 1);  // Finally release IO0
                        usj_write("JIG: BOOT OK (DUT in bootloader mode)\r\n");
                    } else if (!strcmp(cmd, "RUN")) {
                        usj_write("JIG: RUN start\r\n");
                        // Robust RUN sequence: IO0 high first, wait 300ms, then EN pulse
                        gpio_set_level(DUT_IO0_PIN, 1);
                        vTaskDelay(pdMS_TO_TICKS(300));  // Wait 300ms with IO0 high
                        gpio_set_level(DUT_EN_PIN, 0);
                        vTaskDelay(pdMS_TO_TICKS(300));  // Hold EN low for 300ms
                        gpio_set_level(DUT_EN_PIN, 1);   // Release EN, keep it high
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
                    } else if (!strcmp(cmd, "GUI_REQUEST_DATA")) {
                        usj_write("\r\n========================================\r\n");
                        usj_write("JIG: Received !GUI_REQUEST_DATA\r\n");
                        usj_write("JIG: Starting test sequence...\r\n");
                        usj_write("========================================\r\n");
                        
                        // Trigger RS485 and CAN burst transmission (20 messages each)
                        rs485_tx_trigger();
                        can_tx_trigger();
                        usj_write("[RS485/CAN] Triggered 20 message burst\r\n");
                        
                        // Step 1: Run IM test
                        bool im_ok = run_im_test();
                        
                        // Step 2: Read voltage
                        usj_write("\r\n[VOLTAGE ADC] Starting...\r\n");
                        uint16_t voltages[8] = {0};
                        bool voltage_ok = voltage_adc_read_all(voltages);
                        
                        if (voltage_ok) {
                            voltage_adc_send_payload(voltages);
                            usj_write("[VOLTAGE ADC] Payload sent to GUI\r\n");
                        } else {
                            usj_write("[VOLTAGE ADC] Read FAILED\r\n");
                        }
                        
                        // Summary
                        usj_write("\r\n========================================\r\n");
                        char summary[128];
                        snprintf(summary, sizeof(summary), 
                                "JIG: Test complete - IM:%s, Voltage:%s\r\n",
                                im_ok ? "PASS" : "FAIL",
                                voltage_ok ? "OK" : "FAIL");
                        usj_write(summary);
                        usj_write("JIG: Waiting for next !GUI_REQUEST_DATA...\r\n");
                        usj_write("========================================\r\n\r\n");
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

void app_main(void)
{
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "Test Jig Firmware - DUT Control + Voltage ADC");
    ESP_LOGI(TAG, "DUT Ctrl : EN=%d IO0=%d", DUT_EN_PIN, DUT_IO0_PIN);
    ESP_LOGI(TAG, "Voltage  : UART1 TX=%d RX=%d", VOLTAGE_UART_TX, VOLTAGE_UART_RX);
    ESP_LOGI(TAG, "========================================");

    // Configure GPIOs (Push-Pull Output mode)
    gpio_config_t out_conf = {
        .pin_bit_mask = (1ULL << DUT_EN_PIN) | (1ULL << DUT_IO0_PIN) | (1ULL << IGN_TEST_PIN),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    // Set drive strength to maximum for push-pull output
    ESP_ERROR_CHECK(gpio_config(&out_conf));
    ESP_ERROR_CHECK(gpio_set_drive_capability(DUT_EN_PIN, GPIO_DRIVE_CAP_3));
    ESP_ERROR_CHECK(gpio_set_drive_capability(DUT_IO0_PIN, GPIO_DRIVE_CAP_3));
    ESP_ERROR_CHECK(gpio_set_drive_capability(IGN_TEST_PIN, GPIO_DRIVE_CAP_3));
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

    // Initialize and start RS485 TX task (won't affect other functionality)
    ESP_LOGI(TAG, "Initializing RS485 TX module...");
    if (rs485_tx_init()) {
        rs485_tx_start();
        ESP_LOGI(TAG, "RS485 TX started");
    } else {
        ESP_LOGW(TAG, "RS485 TX init failed (RS485 disabled)");
    }

    // Initialize and start CAN TX task (sends 01..08 periodically)
    ESP_LOGI(TAG, "Initializing CAN TX module...");
    if (can_tx_init()) {
        can_tx_start();
        ESP_LOGI(TAG, "CAN TX started");
    } else {
        ESP_LOGW(TAG, "CAN TX init failed (CAN disabled)");
    }

    // Start console task
    ESP_LOGI(TAG, "Creating console task...");
    xTaskCreate(console_task, "console", 4096, NULL, 5, NULL);
    
    // 不再自动周期读取电压和IM测试，仅响应 !GUI_REQUEST_DATA

    // Start IGN test task (continuously toggle IGN_TEST_PIN every 100ms)
    ESP_LOGI(TAG, "Creating IGN test task...");
    BaseType_t ret = xTaskCreate(ign_test_task, "ign_test", 2048, NULL, 3, NULL);
    if (ret == pdPASS) {
        ESP_LOGI(TAG, "IGN test task created successfully");
    } else {
        ESP_LOGE(TAG, "FAILED to create IGN test task!");
    }

    ESP_LOGI(TAG, "Control running. Commands: !BOOT !RUN !RST !VOLTAGE !GUI_REQUEST_DATA");
    ESP_LOGI(TAG, "Handshake mode: send !GUI_REQUEST_DATA to trigger IM test + voltage reading");
    ESP_LOGI(TAG, "IGN test auto-toggling enabled (every 100ms on GPIO%d)", IGN_TEST_PIN);
    ESP_LOGI(TAG, "IM test will run on-demand when !GUI_REQUEST_DATA received");
    ESP_LOGI(TAG, "========================================");
}
