// SPDX-License-Identifier: MIT
// K10-3U8 8-channel voltage acquisition module driver
// Modbus RTU protocol via UART1

#ifndef VOLTAGE_ADC_H
#define VOLTAGE_ADC_H

#include <stdint.h>
#include <stdbool.h>

// UART1 pins for voltage module
#ifndef VOLTAGE_UART_TX
#define VOLTAGE_UART_TX  17  // S3 GPIO17 -> Module RX
#endif
#ifndef VOLTAGE_UART_RX
#define VOLTAGE_UART_RX  18  // S3 GPIO18 -> Module TX
#endif

// Modbus RTU settings
#define VOLTAGE_UART_NUM     UART_NUM_1
#define VOLTAGE_UART_BAUD    9600
#define VOLTAGE_BUF_SIZE     256

// K10-3U8 default address
#ifndef VOLTAGE_MODBUS_ADDR
#define VOLTAGE_MODBUS_ADDR  0x01
#endif

// Number of voltage channels
#define VOLTAGE_CHANNEL_COUNT  8

// Calibration structure for each channel
typedef struct {
    float gain;      // Gain correction factor (通常接近 1.0)
    int16_t offset;  // Offset in mV (零点偏移)
} voltage_cal_t;

// Initialize voltage acquisition module
void voltage_adc_init(void);

// Set calibration parameters for a specific channel (0-7)
// Example: voltage_adc_set_calibration(0, 1.0, -181);  // AI1: gain=1.0, offset=-181mV
void voltage_adc_set_calibration(uint8_t channel, float gain, int16_t offset_mv);

// Load default calibration (based on your measurements)
// AI1: -181mV offset, AI4: -8mV offset
void voltage_adc_load_default_calibration(void);

// Read all 8 channels (blocking, returns true if success)
// voltages[8]: output array in mV (millivolts)
bool voltage_adc_read_all(uint16_t voltages[8]);

// Send voltage data via USB-Serial-JTAG as JSON payload
void voltage_adc_send_payload(const uint16_t voltages[8]);

#endif // VOLTAGE_ADC_H
