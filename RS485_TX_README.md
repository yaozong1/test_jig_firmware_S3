# Test Jig RS485 Configuration

## Overview
治具(ESP32-S3)通过RS485串口向DUT发送周期性测试数据,用于验证DUT的RS485接收功能。

## Hardware Connection

### Jig Side (ESP32-S3)
- **UART2 TX (GPIO15)** → RS485 transceiver DI (Data Input)
- **UART2 RX (GPIO16)** → RS485 transceiver RO (Receive Output)
- RS485 transceiver A/B → Connect to DUT's RS485 A/B

### DUT Side (ESP32)
- **UART0 TX (GPIO10)** → RS485 transceiver DI
- **UART0 RX (GPIO9)** → RS485 transceiver RO
- RS485 transceiver A/B → Connect to Jig's RS485 A/B

## Communication Parameters
- **Baudrate**: 115200 bps
- **Data bits**: 8
- **Parity**: None
- **Stop bits**: 1

## Data Transmission
治具每隔 **1秒** 自动发送以下十六进制数据:
```
01 02 03 04 05 06 07 08
```

## Notes
1. UART端口分配:
   - UART0: USB-Serial-JTAG (Console)
   - UART1: Voltage ADC module (GPIO17/18)
   - **UART2: RS485 TX module (GPIO15/16)**

2. 确保RS485收发器的A/B线正确连接(A-A, B-B)

3. DUT应配置为接收模式,监听治具发送的测试数据

## Testing
DUT可以通过以下方式验证接收:
1. 配置UART0为RS485模式(115200 baud)
2. 每秒应接收到 8 字节数据: `01 02 03 04 05 06 07 08`
3. 可以通过日志输出验证接收的数据内容
