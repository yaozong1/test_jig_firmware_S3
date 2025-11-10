// SPDX-License-Identifier: MIT
// K10-3U8 8-channel voltage acquisition module driver
// Modbus RTU protocol implementation

#include "voltage_adc.h"
#include <string.h>
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "driver/usb_serial_jtag.h"
#include "esp_log.h"

// 外部变量：IM测试结果（在main.c中定义）
extern bool im_test_result;
extern bool im_test_done;

static const char *TAG = "VADC";

// Per-channel calibration data (AI1-AI8)
static voltage_cal_t channel_cal[VOLTAGE_CHANNEL_COUNT] = {
    {1.0f, 0},  // AI1
    {1.0f, 0},  // AI2
    {1.0f, 0},  // AI3
    {1.0f, 0},  // AI4
    {1.0f, 0},  // AI5
    {1.0f, 0},  // AI6
    {1.0f, 0},  // AI7
    {1.0f, 0},  // AI8
};

// Modbus RTU CRC16 calculation
static uint16_t modbus_crc16(const uint8_t *buf, size_t len)
{
    uint16_t crc = 0xFFFF;
    for (size_t i = 0; i < len; i++) {
        crc ^= buf[i];
        for (int j = 0; j < 8; j++) {
            if (crc & 0x0001) {
                crc = (crc >> 1) ^ 0xA001;
            } else {
                crc >>= 1;
            }
        }
    }
    return crc;
}

void voltage_adc_init(void)
{
    ESP_LOGI(TAG, "Initializing K10-3U8 voltage module on UART1");
    ESP_LOGI(TAG, "TX=%d, RX=%d, Baud=%d, Addr=0x%02X", 
             VOLTAGE_UART_TX, VOLTAGE_UART_RX, VOLTAGE_UART_BAUD, VOLTAGE_MODBUS_ADDR);

    // Configure UART1
    uart_config_t uart_config = {
        .baud_rate = VOLTAGE_UART_BAUD,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    
    ESP_ERROR_CHECK(uart_driver_install(VOLTAGE_UART_NUM, VOLTAGE_BUF_SIZE * 2, 0, 0, NULL, 0));
    ESP_ERROR_CHECK(uart_param_config(VOLTAGE_UART_NUM, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(VOLTAGE_UART_NUM, VOLTAGE_UART_TX, VOLTAGE_UART_RX, 
                                  UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));

    ESP_LOGI(TAG, "Voltage module initialized");
}

bool voltage_adc_read_all(uint16_t voltages[8])
{
    // Modbus RTU command: Read Holding Registers
    // Function code: 0x03
    // K10-3U8 工程量寄存器地址: 40017-40024 (0x0010-0x0017)
    // 工程量采集：0~10V 直接对应 0~10000 (单位 mV)
    // 模拟量输入单位为 mV，不需要额外换算！
    // 注意：AI1 通道存在硬件偏移，需要软件校准（约 -181 mV）
    
    uint8_t tx_buf[8];
    tx_buf[0] = VOLTAGE_MODBUS_ADDR;  // Slave address
    tx_buf[1] = 0x03;                 // Function code: Read Holding Registers
    tx_buf[2] = 0x00;                 // Start address high byte
    tx_buf[3] = 0x10;                 // Start address low byte (0x0010 = 寄存器 40017 = AI1)
    tx_buf[4] = 0x00;                 // Register count high byte
    tx_buf[5] = 0x08;                 // Register count low byte (8 channels)
    
    uint16_t crc = modbus_crc16(tx_buf, 6);
    tx_buf[6] = crc & 0xFF;           // CRC low byte
    tx_buf[7] = (crc >> 8) & 0xFF;    // CRC high byte
    
    ESP_LOGI(TAG, ">>> Sending Modbus RTU request (8 bytes):");
    ESP_LOGI(TAG, "    Addr=0x%02X Func=0x%02X Start=0x%04X Count=%d CRC=0x%04X", 
             tx_buf[0], tx_buf[1], (tx_buf[2] << 8) | tx_buf[3], tx_buf[5], crc);
    ESP_LOG_BUFFER_HEX_LEVEL(TAG, tx_buf, 8, ESP_LOG_INFO);
    
    // Clear RX buffer
    uart_flush_input(VOLTAGE_UART_NUM);
    
    // Send request
    int tx_len = uart_write_bytes(VOLTAGE_UART_NUM, tx_buf, 8);
    if (tx_len != 8) {
        ESP_LOGE(TAG, "UART write failed: %d bytes sent (expected 8)", tx_len);
        return false;
    }
    ESP_LOGI(TAG, "    TX OK: %d bytes", tx_len);
    
    // Wait for response
    // Expected response: [Addr][03][Byte_count][Data...][CRC_L][CRC_H]
    // Byte_count = 8 channels * 2 bytes = 16
    // Total: 1 + 1 + 1 + 16 + 2 = 21 bytes
    
    ESP_LOGI(TAG, "    Waiting for response (timeout 500ms)...");
    
    uint8_t rx_buf[32];
    int rx_len = 0;
    int timeout_ms = 500;
    int wait_ms = 0;
    
    while (rx_len < 21 && wait_ms < timeout_ms) {
        int len = uart_read_bytes(VOLTAGE_UART_NUM, rx_buf + rx_len, 
                                  sizeof(rx_buf) - rx_len, pdMS_TO_TICKS(10));
        if (len > 0) {
            rx_len += len;
            ESP_LOGI(TAG, "    Received +%d bytes, total: %d", len, rx_len);
        } else {
            wait_ms += 10;
        }
    }
    
    if (rx_len == 0) {
        ESP_LOGE(TAG, "<<< No response from module (timeout)");
        ESP_LOGE(TAG, "    Check: 1) Wiring (TX<->RX crossed?) 2) Module power 3) Baud rate");
        return false;
    }
    
    if (rx_len < 5) {
        ESP_LOGE(TAG, "<<< Response too short: %d bytes", rx_len);
        ESP_LOG_BUFFER_HEX_LEVEL(TAG, rx_buf, rx_len, ESP_LOG_ERROR);
        return false;
    }
    
    ESP_LOGI(TAG, "<<< Received Modbus RTU response (%d bytes):", rx_len);
    ESP_LOG_BUFFER_HEX_LEVEL(TAG, rx_buf, rx_len, ESP_LOG_INFO);
    
    // Verify response
    if (rx_buf[0] != VOLTAGE_MODBUS_ADDR) {
        ESP_LOGE(TAG, "Address mismatch: expected 0x%02X, got 0x%02X", 
                 VOLTAGE_MODBUS_ADDR, rx_buf[0]);
        return false;
    }
    
    if (rx_buf[1] != 0x03) {
        ESP_LOGE(TAG, "Function code mismatch: expected 0x03, got 0x%02X", rx_buf[1]);
        // Check for Modbus exception
        if (rx_buf[1] & 0x80) {
            ESP_LOGE(TAG, "Modbus exception code: 0x%02X", rx_buf[2]);
        }
        return false;
    }
    
    uint8_t byte_count = rx_buf[2];
    if (byte_count != 16) {
        ESP_LOGE(TAG, "Byte count mismatch: expected 16, got %d", byte_count);
        return false;
    }
    
    if (rx_len < 3 + byte_count + 2) {
        ESP_LOGE(TAG, "Incomplete response: expected %d bytes, got %d", 
                 3 + byte_count + 2, rx_len);
        return false;
    }
    
    // Verify CRC
    uint16_t expected_crc = modbus_crc16(rx_buf, 3 + byte_count);
    uint16_t received_crc = rx_buf[3 + byte_count] | (rx_buf[3 + byte_count + 1] << 8);
    
    if (expected_crc != received_crc) {
        ESP_LOGE(TAG, "CRC mismatch: expected 0x%04X, got 0x%04X", 
                 expected_crc, received_crc);
        return false;
    }
    
    // Extract voltage data
    // K10-3U8 工程量数据格式：0~10V 直接对应 0~10000 (单位 mV)
    // 模拟量输入单位为 mV，直接读取即可，无需换算！
    
    // 先显示完整的原始数据包用于调试
    ESP_LOGI(TAG, "=== Raw Data Analysis ===");
    ESP_LOGI(TAG, "Response bytes: Addr=0x%02X Func=0x%02X ByteCount=%d", 
             rx_buf[0], rx_buf[1], rx_buf[2]);
    
    for (int i = 0; i < 8; i++) {
        int idx = 3 + i * 2;
        ESP_LOGI(TAG, "  Reg[%d] offset=%d: 0x%02X 0x%02X", 
                 i, idx, rx_buf[idx], rx_buf[idx + 1]);
    }
    
    ESP_LOGI(TAG, "=== Voltage Values (工程量，单位 mV) ===");
    for (int i = 0; i < 8; i++) {
        // Each register is 2 bytes (big-endian)
        uint16_t raw = (rx_buf[3 + i * 2] << 8) | rx_buf[3 + i * 2 + 1];
        
        // 检测异常值
        if (raw == 0xFFFF || raw == 0xFFF0 || raw > 10000) {
            ESP_LOGW(TAG, "  AI%d: Raw=%5u (0x%04X) -> INVALID/ERROR (超出量程)", i + 1, raw, raw);
            voltages[i] = 0;  // 异常值设为 0
            continue;
        }
        
        // 应用校准参数：calibrated = (raw * gain) + offset
        int32_t calibrated = (int32_t)(raw * channel_cal[i].gain) + channel_cal[i].offset;
        
        // 限制在有效范围内 (0~10000 mV)
        if (calibrated < 0) calibrated = 0;
        if (calibrated > 10000) calibrated = 10000;
        
        voltages[i] = (uint16_t)calibrated;
        
        if (channel_cal[i].gain != 1.0f || channel_cal[i].offset != 0) {
            ESP_LOGI(TAG, "  AI%d: Raw=%5u -> Calibrated=%5u mV (%6.3f V) [Gain=%.4f, Offset=%+d]", 
                     i + 1, raw, voltages[i], voltages[i] / 1000.0, 
                     channel_cal[i].gain, channel_cal[i].offset);
        } else {
            ESP_LOGI(TAG, "  AI%d: Raw=%5u = %5u mV (%6.3f V)", 
                     i + 1, raw, voltages[i], voltages[i] / 1000.0);
        }
    }
    ESP_LOGI(TAG, "======================");
    
    return true;
}

// Set calibration parameters for a specific channel
void voltage_adc_set_calibration(uint8_t channel, float gain, int16_t offset_mv)
{
    if (channel >= VOLTAGE_CHANNEL_COUNT) {
        ESP_LOGE(TAG, "Invalid channel %d (max: %d)", channel, VOLTAGE_CHANNEL_COUNT - 1);
        return;
    }
    
    channel_cal[channel].gain = gain;
    channel_cal[channel].offset = offset_mv;
    
    ESP_LOGI(TAG, "Set calibration for AI%d: Gain=%.4f, Offset=%+d mV", 
             channel + 1, gain, offset_mv);
}

// Load default calibration based on your test measurements
// AI1 measured 4801 mV for 4620 mV input -> offset = -181 mV
// AI4 measured 4612 mV for 4620 mV input -> offset = -8 mV (very close, within tolerance)
void voltage_adc_load_default_calibration(void)
{
    ESP_LOGI(TAG, "Loading default calibration parameters...");
    
    // AI1: 需要 -181 mV 校正
    voltage_adc_set_calibration(0, 1.0f, -181);
    
    // AI4: 仅偏差 -8 mV，可以忽略或校正
    // voltage_adc_set_calibration(3, 1.0f, 8);  // 可选
    
    // 其他通道如果有测试数据，也可以添加校准参数
    // 例如：voltage_adc_set_calibration(1, 1.0f, 0);
    
    ESP_LOGI(TAG, "Calibration loaded. Test your channels and adjust as needed.");
}

void voltage_adc_send_payload(const uint16_t voltages[8])
{
    // Send as JSON payload via USB-Serial-JTAG, including IM test result
    char payload[300];
    int len = snprintf(payload, sizeof(payload),
        "VOLTAGE_ADC: {"
        "\"ch1\":%u,\"ch2\":%u,\"ch3\":%u,\"ch4\":%u,"
        "\"ch5\":%u,\"ch6\":%u,\"ch7\":%u,\"ch8\":%u,"
        "\"im_tested\":%s,\"im_pass\":%s"
        "}\r\n",
        voltages[0], voltages[1], voltages[2], voltages[3],
        voltages[4], voltages[5], voltages[6], voltages[7],
        im_test_done ? "true" : "false",
        im_test_result ? "true" : "false"
    );
    
    if (len > 0 && len < sizeof(payload)) {
        usb_serial_jtag_write_bytes((const uint8_t*)payload, len, pdMS_TO_TICKS(100));
        ESP_LOGI(TAG, "Sent payload: %s", payload);
    }
}
