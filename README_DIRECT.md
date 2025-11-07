# Test Jig Firmware - Control Only Mode

## 架构

**简化版治具固件**，只负责控制 DUT 的 EN 和 IO0 引脚，不再进行 UART 透传。

- **CH340 直连 DUT UART0**：烧录和监听都直接通过 CH340
- **S3 只负责控制**：通过 USB-Serial-JTAG 接收 PC 命令（!BOOT/!RUN/!RST），控制 DUT 的 EN/IO0

## 硬件连接

### S3 治具端
- **USB-Serial-JTAG**（COM24）→ PC，用于控制命令
- **GPIO4** → DUT EN（使能）
- **GPIO5** → DUT IO0（进入下载模式）
- **GND** → DUT GND

### CH340 端
- **CH340 TX** → **DUT U0RXD (GPIO44)**
- **CH340 RX** ← **DUT U0TXD (GPIO43)**
- **CH340 GND** → DUT GND
- CH340 的 DTR/RTS 不接（由 S3 GPIO 控制）

### 不再需要的连线
- ~~S3 UART1 IO17/18 到 DUT~~（删除）
- ~~S3 UART2 IO11/12 到 CH340~~（删除）

## 编译和烧录

```powershell
cd tools\test_jig_firmware
idf.py build
idf.py -p COM24 flash monitor
```

## 控制命令

通过 USB-Serial-JTAG 串口（COM24）发送：

- `!BOOT` - 让 DUT 进入下载模式（IO0=0 + EN 脉冲）
- `!RUN` - 让 DUT 启动应用（IO0=1 + EN 脉冲）
- `!RST` - 复位 DUT（EN 脉冲）

## 使用流程

1. **烧录**：
   - PC → S3 (COM24): `!BOOT`
   - PC → CH340 (COM6): `esptool write_flash ...`（可用高波特率 921600）
   
2. **运行并读取自测**：
   - PC → S3 (COM24): `!RUN`
   - PC ← CH340 (COM6): 读取 DUT 打印的 SELFTEST SUMMARY JSON

## 优势

- ✅ **更快**：直连无透传损耗，可用 921600 甚至更高波特率
- ✅ **更简单**：S3 固件无需 UART 配置，只控制 GPIO
- ✅ **更稳定**：减少中间环节，降低故障点
- ✅ **更省资源**：S3 无需 UART 任务，降低 CPU/内存开销
