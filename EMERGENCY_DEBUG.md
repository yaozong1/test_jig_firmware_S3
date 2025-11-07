# 紧急排查：COM24 无输出

## 立即检查清单

### 1. 确认设备连接
打开 **设备管理器** (Win+X → 设备管理器)，查找：

```
端口 (COM 和 LPT)
├─ USB-JTAG/serial debug unit (COMxx)  ← 这个是 ESP32-S3 的 USB-Serial-JTAG
```

**问题**：
- ❌ 如果看不到这个设备 → USB 线有问题或 S3 没上电
- ❌ 如果显示黄色感叹号 → 驱动问题
- ✅ 如果显示正常 → 记下 COM 号（可能不是 COM24）

### 2. 确认 COM 端口号
你的 S3 的 USB-Serial-JTAG 可能**不是 COM24**！

**解决方案**：
1. 在设备管理器中找到实际的 COM 号
2. 串口助手连接到**正确的 COM 号**
3. 如果是其他号码（如 COM5），后续命令改用正确的号码

### 3. 检查串口参数
串口助手设置必须是：
- 波特率: **115200**
- 数据位: 8
- 停止位: 1
- 校验位: 无
- 流控: 无

### 4. 检查是否被其他程序占用
**关闭所有可能占用串口的程序**：
- idf.py monitor
- 其他串口助手
- Python 脚本（如果在运行）
- Arduino IDE
- PuTTY

### 5. 复位 ESP32-S3
**物理操作**：
1. 拔掉 S3 的 USB 线
2. 等待 5 秒
3. 重新插上 USB 线
4. 立即在串口助手中观察

**应该看到**：
- ESP32-S3 上电时的启动信息
- ROM bootloader 信息
- 应用程序启动日志

## 如果仍然没有输出

### 测试 1: 使用 esptool 测试连接

在普通 PowerShell 中运行：

```powershell
python -m esptool --port COM24 chip_id
```

**替换 COM24 为你的实际端口号**

**预期结果**：
```
esptool.py v4.x
Serial port COM24
Connecting....
Detecting chip type... ESP32-S3
Chip is ESP32-S3 (revision v0.1)
...
```

**如果失败**：
- 提示 "serial port not found" → COM 号错误
- 提示 "failed to connect" → S3 未进入下载模式或USB问题

### 测试 2: 强制进入下载模式

**手动操作**（如果 S3 有 BOOT 按钮）：
1. 按住 BOOT 按钮
2. 按一下 RESET 按钮
3. 松开 BOOT 按钮
4. 运行 `python -m esptool --port COMxx chip_id`

### 测试 3: 检查固件是否烧录

**可能的问题**：
- ❌ 从未烧录过固件
- ❌ 烧录失败但没注意到错误
- ❌ 烧录到错误的端口

**验证方法**：
在 ESP-IDF PowerShell 中：

```powershell
cd C:\Users\h1576\Desktop\Roam\pe_code\pe_board\tools\test_jig_firmware

# 先检测芯片（确认连接）
esptool.py --port COM24 chip_id

# 如果成功，尝试读取 flash
esptool.py --port COM24 read_flash 0x0 0x1000 test.bin
```

如果这些命令都失败，说明根本连不上 S3。

## 快速修复方案

### 方案 A: 重新烧录 Bootloader

可能 bootloader 损坏或不完整：

```powershell
cd C:\Users\h1576\Desktop\Roam\pe_code\pe_board\tools\test_jig_firmware
idf.py -p COM24 erase-flash
idf.py -p COM24 flash
```

### 方案 B: 使用出厂固件测试

烧录一个最简单的固件测试硬件：

**创建测试固件** (我来帮你创建一个超简单的)

### 方案 C: 检查 USB 线

**某些 USB 线只能充电不能传数据！**

**测试方法**：
1. 换一根 USB 数据线
2. 换一个 USB 口（直接插主板，不要用 HUB）

## 我现在帮你创建最小测试固件

我会创建一个最简单的固件，只输出 "Hello" 到串口，用来测试硬件是否正常。
