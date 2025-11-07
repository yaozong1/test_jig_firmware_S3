# Test Jig Firmware (ESP32-S3)# Test Jig Firmware (ESP32-S3)



这是一个基于 ESP32-S3 的测试工具固件，用于通过 TinyUSB CDC 桥接功能对 DUT (Device Under Test) 进行烧录和测试。This is an ESP-IDF project for the factory test jig based on ESP32-S3.



## 功能特性Two roles:

- CDC↔UART Bridge (TinyUSB CDC): PC 上会出现一个“CDC COM 口”，本口桥接到 DUT 的 U0TXD/U0RXD；同时把 CDC 的 DTR/RTS 映射为 DUT 的 EN/IO0，支持 esptool/idf.py 直接对 DUT 烧录与复位。

- **CDC ↔ UART 桥接**：PC 上会出现一个 CDC 串口，用于与 DUT 的 UART (U0TXD/U0RXD) 通信- （可选）USB-Serial-JTAG：治具自身的日志口，仅用于开发调试，不参与 DUT 烧录。

- **DTR/RTS 控制**：通过 CDC 的 DTR/RTS 信号控制 DUT 的 EN/IO0 引脚，支持 esptool 直接对 DUT 烧录与复位

- **自定义命令**：支持通过 `!` 前缀发送治具命令（BOOT、RUN、RST）## Wiring (CDC Bridge)



## 硬件连接- DUT U0TXD (GPIO43) -> Jig UART1 RX (默认 GPIO17)

- DUT U0RXD (GPIO44) <- Jig UART1 TX (默认 GPIO18)

### DUT 连接（CDC 桥接）- DUT EN             <- Jig GPIO4（由 CDC 的 DTR 控制，断言=低电平）

- DUT IO0            <- Jig GPIO5（由 CDC 的 RTS 控制，断言=低电平）

| DUT 引脚 | 功能 | 治具引脚 | 默认GPIO |- GND                <-> GND（共地）

|---------|------|----------|---------|

| U0TXD (GPIO43) | TX | UART RX | GPIO17 |可在 `main/main.c` 中调整 `JIG_UART_RX_PIN` / `JIG_UART_TX_PIN` / `DUT_EN_PIN` / `DUT_IO0_PIN`。

| U0RXD (GPIO44) | RX | UART TX | GPIO18 |

| EN | 复位控制 | EN控制 | GPIO4 |## Build & Flash

| IO0 | 启动模式 | IO0控制 | GPIO5 |

| GND | 地 | GND | 共地 |1) Ensure ESP-IDF (v5.x) is installed and environment activated.

2) From this folder:

可在 `main/main.c` 中修改以下宏来调整引脚：

```c```powershell

#define JIG_UART_TX_PIN    18idf.py set-target esp32s3

#define JIG_UART_RX_PIN    17idf.py build

#define DUT_EN_PIN         4idf.py -p COMx flash monitor

#define DUT_IO0_PIN        5```

```

Replace `COMx` with the COM port of the jig's USB-Serial-JTAG（用于烧录治具自身）。

## 构建和烧录

启用 TinyUSB CDC（一次性设置）：

### 前提条件

1) `idf.py menuconfig`

1. 安装 ESP-IDF v5.0 或更高版本2) Component config -> TinyUSB：

2. 激活 ESP-IDF 环境：	- [*] Enable TinyUSB device stack

   ```powershell	- [*] CDC support (CDC-ACM)

   C:\Users\<你的用户名>\esp\esp-idf\export.ps13) 保存退出，重新 `idf.py build` 和 `idf.py -p COMx flash`。

   ```

PC 端会新增一个 “USB 串行设备 (COMy)”（CDC 口）。该口就是“直连 DUT U0TXD/U0RXD 的桥”，可被 `esptool.py` / `idf.py -p COMy flash` 或你的 Python GUI 用于烧录/交互 DUT。

### 构建固件

## Notes

```powershell

cd tools/test_jig_firmware- 烧录 DUT 时，请使用 CDC 口（桥口）；日志口（USB-Serial-JTAG）不要同时占用同一 CDC 口。

idf.py set-target esp32s3- 若 CDC 口下烧录失败，检查 EN/IO0 接线与极性，DTR/RTS 断言语义为“有效低”。

idf.py build- 速率默认 115200，可按需在 `main.c` 的 `JIG_UART_BAUD` 调整。

```

### 烧录固件

使用 USB-Serial-JTAG 接口烧录治具固件：

```powershell
idf.py -p COMx flash
```

将 `COMx` 替换为治具的 USB-Serial-JTAG 端口号。

### 查看日志（可选）

```powershell
idf.py -p COMx monitor
```

## 使用方法

### 1. 连接硬件
按照上述硬件连接表连接治具与 DUT。

### 2. 连接 USB
将治具通过 USB 连接到 PC，PC 会识别出一个 CDC 串口设备（例如 COM7）。

### 3. 烧录 DUT

使用 esptool 或 idf.py 通过治具的 CDC 端口烧录 DUT：

```powershell
# 使用 idf.py
idf.py -p COMy flash

# 或使用 esptool
esptool.py -p COMy write_flash 0x10000 your_firmware.bin
```

**注意**：`COMy` 是治具的 CDC 端口，**不是** USB-Serial-JTAG 端口。

### 4. 自定义命令（可选）

通过 CDC 端口发送以下命令（命令以 `!` 开头）：

- `!BOOT` - 让 DUT 进入下载模式
- `!RUN` - 正常复位 DUT
- `!RST` - 简单复位 DUT

例如，使用串口工具发送：`!BOOT\r\n`

治具会回复：
```
JIG: BOOT start
JIG: BOOT OK (DUT ready to flash)
```

## DTR/RTS 控制逻辑

治具实现了标准的 ESP32 复位逻辑：

- **DTR=1 → EN=0**（EN 拉低，准备复位）
- **DTR=0 → EN=1**（EN 拉高，正常运行）
- **RTS=1 → IO0=0**（IO0 拉低，进入下载模式）
- **RTS=0 → IO0=1**（IO0 拉高，正常启动）

esptool 会自动控制这些信号来进入下载模式或复位 DUT。

## 配置选项

### 修改波特率

在 `main/main.c` 中修改：
```c
#define JIG_UART_BAUD      115200  // 改为其他波特率，如 921600
```

### 修改 USB VID/PID

在 `main/usb_descriptors.c` 中修改：
```c
.idVendor           = 0x303A,  // Espressif VID
.idProduct          = 0x4001,  // 自定义 PID
```

### 禁用自定义命令

如果不需要自定义命令功能，可以在 `main/main.c` 的 `usb_to_uart_task` 函数中注释掉命令解析逻辑。

## 故障排除

### PC 无法识别 CDC 端口
1. 检查 TinyUSB 是否正确配置（应该已启用）
2. 重新烧录固件
3. 尝试重新插拔 USB 电缆
4. 在设备管理器中检查是否有未知设备

### 无法烧录 DUT
1. 检查治具与 DUT 的硬件连接
2. 确认使用的是 CDC 端口（不是 USB-Serial-JTAG）
3. 检查 EN 和 IO0 的连接和逻辑电平
4. 尝试手动发送 `!BOOT` 命令后再烧录

### 编译错误
1. 确保 ESP-IDF 版本为 v5.0 或更高
2. 确保 `tusb_config.h` 已复制到 `managed_components/espressif__tinyusb/src/` 目录
3. 运行 `idf.py fullclean` 后重新构建

## 技术细节

- **芯片**：ESP32-S3
- **USB 控制器**：TinyUSB
- **CDC 接口数**：1（用于 DUT 桥接）
- **UART**：UART1（GPIO17/18）
- **缓冲区大小**：CDC RX/TX 512 字节，UART RX 4096 字节，TX 2048 字节

## 许可证

MIT License

## 版本历史

- **v1.0** (2025-01)
  - 初始版本
  - 支持 TinyUSB CDC 桥接
  - DTR/RTS 控制 DUT EN/IO0
  - 自定义命令支持
