# Test Jig Firmware (ESP32-S3)

这是基于 ESP32-S3 的测试治具固件，用于控制被测设备 (DUT) 并采集电压数据。

## 功能特性

### 1. DUT 控制
- **EN 控制** (GPIO4)：控制 DUT 复位
- **IO0 控制** (GPIO5)：控制 DUT 启动模式（下载/运行）
- 支持命令：`!BOOT`、`!RUN`、`!RST`

### 2. 8通道电压采集
- **硬件**：K10-3U8 电压采集模块
- **协议**：Modbus RTU (9600 baud)
- **接口**：UART1 (GPIO17 TX, GPIO18 RX)
- **功能**：自动每 2 秒读取 8 个通道电压值
- 支持命令：`!VOLTAGE`（手动读取）

### 3. USB-Serial-JTAG 控制台
- **端口**：USB-Serial-JTAG (通常识别为 COM24)
- **用途**：
  - 接收来自 PC 的控制命令
  - 输出日志和调试信息
  - 发送电压采集数据（JSON 格式）

## 硬件连接

### 治具 S3 → DUT
```
GPIO4  → DUT EN   (复位控制)
GPIO5  → DUT IO0  (启动模式)
GND    → DUT GND  (共地)
```

### 治具 S3 → K10-3U8 电压模块
```
GPIO17 (TX) → 模块 RX
GPIO18 (RX) ← 模块 TX
GND         → 模块 GND
5V          → 模块 VCC (可选，视模块供电方式)
```

### 治具 S3 → PC
```
USB-C → PC  (USB-Serial-JTAG, 用于控制和数据传输)
```

## 编译和烧录

### 前置条件
- ESP-IDF v5.x (推荐 v5.1.2)
- 已激活 ESP-IDF 环境

### 编译步骤

**方法 1：使用 ESP-IDF 命令提示符**
```cmd
cd tools\test_jig_firmware
idf.py build
idf.py -p COM24 flash monitor
```

**方法 2：使用批处理脚本**
```cmd
tools\test_jig_firmware\build_and_flash.bat
```

## 使用方法

### 通过 USB-Serial-JTAG 发送命令

打开串口工具（波特率 115200），连接到 COM24，发送以下命令：

- **`!BOOT`**：让 DUT 进入下载模式（IO0=LOW, 复位）
- **`!RUN`**：让 DUT 正常运行（IO0=HIGH, 复位）
- **`!RST`**：复位 DUT
- **`!VOLTAGE`**：手动读取一次电压值

### 自动电压监控

固件启动后会自动每 2 秒读取一次电压，无需手动发送命令。

### 输出格式

**启动信息**：
```
========================================
Test Jig Firmware - DUT Control + Voltage ADC
DUT Ctrl : EN=4 IO0=5
Voltage  : UART1 TX=17 RX=18
========================================
```

**电压数据（JSON）**：
```
VOLTAGE_ADC: {"ch1":3300,"ch2":5000,"ch3":0,"ch4":0,"ch5":0,"ch6":0,"ch7":0,"ch8":0}
```

**调试信息（Modbus）**：
```
>>> Sending Modbus RTU request (8 bytes):
    Addr=0x01 Func=0x03 Start=0x0000 Count=8
01 03 00 00 00 08 44 0C
    TX OK: 8 bytes
    Waiting for response (timeout 500ms)...
    Received +21 bytes, total: 21
<<< Received Modbus RTU response (21 bytes):
01 03 10 0C E4 13 88 00 00 ...
=== Voltage Values ===
  CH1:  3300 mV (  3.300 V)
  CH2:  5000 mV (  5.000 V)
  ...
======================
```

## 配置选项

### 修改引脚定义

编辑 `main/main.c`：
```c
#define DUT_EN_PIN          4   // DUT 复位引脚
#define DUT_IO0_PIN         5   // DUT IO0 引脚
```

编辑 `main/voltage_adc.h`：
```c
#define VOLTAGE_UART_TX     17  // 连接模块 RX
#define VOLTAGE_UART_RX     18  // 连接模块 TX
```

### 修改读取间隔

编辑 `main/main.c` 中的 `voltage_task`：
```c
vTaskDelay(pdMS_TO_TICKS(2000)); // 改为你想要的毫秒数
```

### 调整 Modbus 参数

编辑 `main/voltage_adc.h`：
```c
#define VOLTAGE_MODBUS_ADDR     0x01  // 模块地址
#define VOLTAGE_UART_BAUD       9600  // 波特率
```

## 故障排除

### COM24 没有输出

**解决方案**：
1. 检查 sdkconfig 中控制台配置为 `CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG=y`
2. 重新编译并烧录固件
3. 参考 `FLASH_GUIDE.md`

### 电压读取超时

**可能原因**：
1. TX/RX 接线反了（应该交叉连接）
2. 模块未上电
3. 波特率不匹配
4. Modbus 地址不正确

**解决方案**：
- 参考 `VOLTAGE_ADC_README.md` 检查接线
- 查看 Modbus 调试日志中的详细错误信息

### CRC 校验错误

**可能原因**：
1. 接线质量差
2. 连接线过长
3. 信号干扰

**解决方案**：
- 缩短连接线
- 确保良好的 GND 连接
- 降低波特率测试

## 文档参考

- `VOLTAGE_ADC_README.md` - K10-3U8 模块详细说明
- `VOLTAGE_TEST_GUIDE.md` - 电压采集测试指南
- `FLASH_GUIDE.md` - 固件烧录详细步骤
- `build_and_flash.bat` - 快速编译烧录脚本

## 许可证

MIT License
