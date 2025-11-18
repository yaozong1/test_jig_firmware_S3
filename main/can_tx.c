// SPDX-License-Identifier: MIT
// CAN TX Module for Test Jig - Send periodic data to DUT

#include "can_tx.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/twai.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "CAN_TX";

// CAN configuration for ESP32-S3
// 注意：这里的TX/RX是从ESP32-S3的角度，连接到CAN收发器(如TJA1051)
#ifndef CAN_TX_PIN
#define CAN_TX_PIN          40  // S3 GPIO40 -> CAN收发器TXD (ESP32发送数据到CAN总线)
#endif

#ifndef CAN_RX_PIN
#define CAN_RX_PIN          39  // S3 GPIO39 <- CAN收发器RXD (ESP32从CAN总线接收数据)
#endif
// No CAN_EN/SHUTDOWN pin needed (as per user request)

static bool s_can_inited = false;
static TaskHandle_t s_tx_task_handle = NULL;
static volatile int s_send_count = 0;  // Number of messages to send (0 = idle)

// On-demand transmission task (sends when triggered)
static void can_tx_task(void *arg)
{
    const uint8_t data[] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
    
    ESP_LOGI(TAG, ">>> CAN TX task STARTING (on-demand mode) <<<");
    
    twai_message_t tx_msg;
    tx_msg.identifier = 0x100;  // CAN ID 0x100
    tx_msg.extd = 0;            // Standard frame (11-bit ID)
    tx_msg.rtr = 0;             // Data frame (not remote)
    tx_msg.data_length_code = sizeof(data);
    memcpy(tx_msg.data, data, sizeof(data));
    
    while (1) {
        if (s_send_count > 0) {
            int total_to_send = s_send_count;
            
            // Print start message once
            ESP_LOGI(TAG, "Start sending CAN \"01~08\" (ID=0x100, %d messages)", total_to_send);
            
            // Send all messages
            while (s_send_count > 0) {
                esp_err_t ret = twai_transmit(&tx_msg, pdMS_TO_TICKS(20));  // 超时20ms
                
                // 注释掉超时日志,避免刷屏
                // if (ret != ESP_OK) {
                //     ESP_LOGW(TAG, "Send failed: %s", esp_err_to_name(ret));
                // }
                (void)ret;  // Suppress unused variable warning
                
                s_send_count--;
                
                if (s_send_count > 0) {
                    vTaskDelay(pdMS_TO_TICKS(100));  // 100ms间隔
                }
            }
            
            // Print done message once
            ESP_LOGI(TAG, "Done CAN (%d messages sent)", total_to_send);
        } else {
            // Idle, wait for trigger
            vTaskDelay(pdMS_TO_TICKS(100));
        }
    }
}

bool can_tx_init(void)
{
    if (s_can_inited) {
        ESP_LOGW(TAG, "CAN TX already initialized");
        return true;
    }

    ESP_LOGI(TAG, "Initializing CAN module: TX=%d, RX=%d", CAN_TX_PIN, CAN_RX_PIN);

    // TWAI general configuration - normal mode
    twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT(CAN_TX_PIN, CAN_RX_PIN, TWAI_MODE_NORMAL);
    g_config.intr_flags = 0;  // Default interrupt flags
    
    // TWAI timing configuration - 250Kbps
    twai_timing_config_t t_config = TWAI_TIMING_CONFIG_250KBITS();
    
    // TWAI filter configuration - accept all
    twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();

    // Install TWAI driver
    esp_err_t ret = twai_driver_install(&g_config, &t_config, &f_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to install TWAI driver: %s", esp_err_to_name(ret));
        return false;
    }

    // Start TWAI driver
    ret = twai_start();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start TWAI driver: %s", esp_err_to_name(ret));
        twai_driver_uninstall();
        return false;
    }

    s_can_inited = true;
    ESP_LOGI(TAG, "CAN TX init OK: TX=GPIO%d, RX=GPIO%d, bitrate=250Kbps", 
             CAN_TX_PIN, CAN_RX_PIN);
    
    return true;
}

void can_tx_start(void)
{
    if (!s_can_inited) {
        ESP_LOGE(TAG, "CAN TX not initialized, call can_tx_init() first");
        return;
    }

    if (s_tx_task_handle != NULL) {
        ESP_LOGW(TAG, "CAN TX task already running");
        return;
    }

    // Create on-demand transmission task with larger stack
    BaseType_t ret = xTaskCreate(can_tx_task, "can_tx", 4096, NULL, 3, &s_tx_task_handle);
    if (ret == pdPASS) {
        ESP_LOGI(TAG, "CAN TX task created successfully");
    } else {
        ESP_LOGE(TAG, "Failed to create CAN TX task");
        s_tx_task_handle = NULL;
    }
}

void can_tx_trigger(void)
{
    // Trigger 100 message burst (100ms interval, total 10 seconds)
    s_send_count = 100;
}

void can_tx_stop(void)
{
    if (s_tx_task_handle != NULL) {
        vTaskDelete(s_tx_task_handle);
        s_tx_task_handle = NULL;
        ESP_LOGI(TAG, "CAN TX task stopped");
    }
}
