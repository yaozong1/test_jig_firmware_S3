// SPDX-License-Identifier: MIT
// RS485 TX Module for Test Jig - Send periodic data to DUT

#include "rs485_tx.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "RS485_TX";

// RS485 UART configuration for ESP32-S3
#ifndef RS485_UART_NUM
#define RS485_UART_NUM      UART_NUM_2  // Use UART2 for RS485 (avoid conflict with UART1 voltage ADC)
#endif

#ifndef RS485_TX_PIN
#define RS485_TX_PIN        15  // S3 GPIO15 for RS485 TX
#endif

#ifndef RS485_RX_PIN
#define RS485_RX_PIN        16  // S3 GPIO16 for RS485 RX
#endif

#ifndef RS485_BAUDRATE
#define RS485_BAUDRATE      115200  // Baudrate 115200 bps
#endif

#define RS485_BUF_SIZE      256

static bool s_rs485_inited = false;
static TaskHandle_t s_tx_task_handle = NULL;
static volatile int s_send_count = 0;  // Number of messages to send (0 = idle)

// On-demand transmission task (sends when triggered)
static void rs485_tx_task(void *arg)
{
    const uint8_t data[] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
    
    ESP_LOGI(TAG, ">>> RS485 TX task STARTING (on-demand mode) <<<");
    
    while (1) {
        if (s_send_count > 0) {
            int total_to_send = s_send_count;
            
            // Print start message once
            ESP_LOGI(TAG, "Start sending RS485 \"01~08\" (%d messages)", total_to_send);
            
            // Send all messages
            while (s_send_count > 0) {
                int written = uart_write_bytes(RS485_UART_NUM, (const char*)data, sizeof(data));
                
                if (written != sizeof(data)) {
                    ESP_LOGW(TAG, "Send incomplete: %d/%d bytes", written, sizeof(data));
                }
                
                s_send_count--;
                
                if (s_send_count > 0) {
                    vTaskDelay(pdMS_TO_TICKS(100));  // 100ms between messages
                }
            }
            
            // Print done message once
            ESP_LOGI(TAG, "Done RS485 (%d messages sent)", total_to_send);
        } else {
            // Idle, wait for trigger
            vTaskDelay(pdMS_TO_TICKS(100));
        }
    }
}

bool rs485_tx_init(void)
{
    if (s_rs485_inited) {
        ESP_LOGW(TAG, "RS485 TX already initialized");
        return true;
    }

    // Configure UART
    uart_config_t uart_config = {
        .baud_rate = RS485_BAUDRATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    esp_err_t ret;
    
    // Install UART driver
    ret = uart_driver_install(RS485_UART_NUM, RS485_BUF_SIZE, RS485_BUF_SIZE, 0, NULL, 0);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "uart_driver_install failed: %s", esp_err_to_name(ret));
        return false;
    }

    // Configure UART parameters
    ret = uart_param_config(RS485_UART_NUM, &uart_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "uart_param_config failed: %s", esp_err_to_name(ret));
        uart_driver_delete(RS485_UART_NUM);
        return false;
    }

    // Set UART pins
    ret = uart_set_pin(RS485_UART_NUM, RS485_TX_PIN, RS485_RX_PIN, 
                       UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "uart_set_pin failed: %s", esp_err_to_name(ret));
        uart_driver_delete(RS485_UART_NUM);
        return false;
    }

    s_rs485_inited = true;
    ESP_LOGI(TAG, "RS485 TX init OK: UART%d, TX=GPIO%d, RX=GPIO%d, baud=%d", 
             RS485_UART_NUM, RS485_TX_PIN, RS485_RX_PIN, RS485_BAUDRATE);
    
    return true;
}

void rs485_tx_start(void)
{
    if (!s_rs485_inited) {
        ESP_LOGE(TAG, "RS485 TX not initialized, call rs485_tx_init() first");
        return;
    }

    if (s_tx_task_handle != NULL) {
        ESP_LOGW(TAG, "RS485 TX task already running");
        return;
    }

    // Create on-demand transmission task with larger stack
    BaseType_t ret = xTaskCreate(rs485_tx_task, "rs485_tx", 4096, NULL, 3, &s_tx_task_handle);
    if (ret == pdPASS) {
        ESP_LOGI(TAG, "RS485 TX task created successfully");
    } else {
        ESP_LOGE(TAG, "Failed to create RS485 TX task");
        s_tx_task_handle = NULL;
    }
}

void rs485_tx_trigger(void)
{
    // Trigger 100 message burst (100ms interval, total 10 seconds)
    s_send_count = 100;
}

void rs485_tx_stop(void)
{
    if (s_tx_task_handle != NULL) {
        vTaskDelete(s_tx_task_handle);
        s_tx_task_handle = NULL;
        ESP_LOGI(TAG, "RS485 TX task stopped");
    }
}
