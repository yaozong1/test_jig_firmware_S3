# 治具固件编译和烧录说明

## 问题原因
治具固件的控制台原本配置为 **UART0**，导致所有的 ESP_LOG 输出无法在 USB-Serial-JTAG (COM24) 上看到。

## 解决方案
已将 sdkconfig 修改为：
- ✅ 主控制台：USB-Serial-JTAG (COM24)
- ✅ 次要控制台：无

这样所有日志（包括 voltage_task 的输出）都会显示在 COM24。

## 编译和烧录步骤

### 方法 1：使用 ESP-IDF 命令提示符（推荐）

1. **打开 ESP-IDF 5.1.2 CMD**（从开始菜单）

2. **切换到治具固件目录**：
   ```cmd
   cd C:\Users\h1576\Desktop\Roam\pe_code\pe_board\tools\test_jig_firmware
   ```

3. **编译**：
   ```cmd
   idf.py build
   ```

4. **烧录并监视**：
   ```cmd
   idf.py -p COM24 flash monitor
   ```

5. **查看输出**：
   - 烧录完成后会自动进入监视模式
   - 按 `Ctrl+]` 退出监视

### 方法 2：使用批处理脚本

1. **打开 ESP-IDF 5.1.2 CMD**

2. **运行脚本**：
   ```cmd
   C:\Users\h1576\Desktop\Roam\pe_code\pe_board\tools\test_jig_firmware\build_and_flash.bat
   ```

## 预期输出

启动后应该立即看到：

```
========================================
Test Jig Firmware - DUT Control + Voltage ADC
DUT Ctrl : EN=4 IO0=5
Voltage  : UART1 TX=17 RX=18
========================================
Initializing K10-3U8 voltage module on UART1
TX=17, RX=18, Baud=9600, Addr=0x01
Voltage module initialized
Control running. Commands: !BOOT !RUN !RST !VOLTAGE
Voltage auto-reading every 2 seconds...
Voltage task started, reading every 2 seconds...
```

然后每 2 秒看到：

```
=== Reading Voltage ===
>>> Sending Modbus RTU request (8 bytes):
    Addr=0x01 Func=0x03 Start=0x0000 Count=8 CRC=0x...
01 03 00 00 00 08 XX XX
    TX OK: 8 bytes
    Waiting for response (timeout 500ms)...
```

## 如果仍然没有输出

1. **检查 COM24 是否正确**：
   - 设备管理器中查看端口号
   - 确认是 "USB-JTAG/serial debug unit"

2. **检查其他程序**：
   - 关闭所有串口助手
   - 关闭其他占用 COM24 的程序

3. **重新插拔 USB**：
   - 拔掉 S3 的 USB
   - 等待 5 秒
   - 重新插上

4. **检查驱动**：
   - Windows 11 应该自动识别
   - 如果不行，安装 Silicon Labs CP210x 驱动
