# 最小固件测试步骤

## 用途
这个最小固件只做一件事：每秒输出 "Hello" 到 USB-Serial-JTAG。
用于测试硬件和 USB-Serial-JTAG 是否正常工作。

## 使用步骤

### 1. 备份当前 main.c
```powershell
cd C:\Users\h1576\Desktop\Roam\pe_code\pe_board\tools\test_jig_firmware\main
copy main.c main_backup.c
```

### 2. 替换为测试固件
```powershell
copy main_test_minimal.c main.c
```

### 3. 编译和烧录
在 **ESP-IDF 5.1.2 PowerShell** 中：

```powershell
cd C:\Users\h1576\Desktop\Roam\pe_code\pe_board\tools\test_jig_firmware

# 方法 1: 一条命令完成
idf.py -p COM24 flash monitor

# 方法 2: 分步执行
idf.py build
idf.py -p COM24 flash
idf.py -p COM24 monitor
```

**注意**: 替换 COM24 为你在设备管理器中看到的实际端口号！

### 4. 预期输出

烧录完成后，串口监视器应该立即显示：

```
================================
MINIMAL TEST FIRMWARE
Testing USB-Serial-JTAG output
================================

I (xxx) TEST: ESP32-S3 is ALIVE!
I (xxx) TEST: This is a minimal test
>>> Hello from ESP32-S3! Count=1 <<<
I (xxx) TEST: Loop iteration: 1
>>> Hello from ESP32-S3! Count=2 <<<
I (xxx) TEST: Loop iteration: 2
>>> Hello from ESP32-S3! Count=3 <<<
I (xxx) TEST: Loop iteration: 3
...
```

每秒输出一次，计数递增。

### 5. 如果仍然没有输出

#### A. 检查 idf.py flash 的输出
烧录时应该看到：

```
Connecting.....
Chip is ESP32-S3 (revision v0.1)
...
Wrote XXXXX bytes at 0x00010000 in X.X seconds
...
Hard resetting via RTS pin...
```

如果看到 "Connecting....." 一直卡住，说明：
- COM 端口错误
- USB 线有问题
- S3 硬件问题

#### B. 强制进入下载模式
如果有 BOOT 按钮：
1. 按住 BOOT
2. 按一下 RESET
3. 松开 BOOT
4. 立即运行 `idf.py -p COMxx flash`

#### C. 检查 USB 线
**最常见问题**: USB 线只能充电不能传数据

**解决方案**: 换一根确认能传数据的 USB 线

#### D. 检查 sdkconfig
确认控制台配置：

```powershell
cd C:\Users\h1576\Desktop\Roam\pe_code\pe_board\tools\test_jig_firmware
findstr "CONFIG_ESP_CONSOLE" sdkconfig
```

应该看到：
```
CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG=y
```

如果不是，运行：
```powershell
idf.py menuconfig
# Component config -> ESP System Settings -> Channel for console output
# 选择: USB Serial/JTAG Controller
# 保存并退出
idf.py build
idf.py -p COM24 flash
```

### 6. 恢复原固件

测试完成后：

```powershell
cd C:\Users\h1576\Desktop\Roam\pe_code\pe_board\tools\test_jig_firmware\main
copy main_backup.c main.c
```

然后重新编译烧录。

## 故障排除决策树

```
串口助手无输出
├─ 设备管理器看不到 "USB-JTAG/serial debug unit"
│  ├─ 换 USB 线
│  ├─ 换 USB 口（直接插主板）
│  └─ 检查 S3 是否上电（LED灯？）
│
├─ 看得到设备但有黄色感叹号
│  └─ 重新安装驱动（Windows Update 或 CP210x）
│
├─ 设备正常但 COM 号不是 COM24
│  └─ 使用实际的 COM 号（如 COM5、COM8 等）
│
├─ esptool chip_id 失败
│  ├─ COM 号错误 → 用正确的号
│  ├─ 串口被占用 → 关闭所有串口工具
│  └─ 进入下载模式 → 按 BOOT+RESET
│
└─ esptool 成功但烧录后无输出
   ├─ sdkconfig 控制台配置错误 → 运行 menuconfig
   ├─ bootloader 问题 → erase-flash 后重新烧录
   └─ 固件卡死 → 用最小测试固件验证
```

## 联系我

测试后请告诉我：
1. `idf.py flash` 是否成功？贴出输出
2. 设备管理器中的实际 COM 号是多少？
3. 最小测试固件是否有输出？
4. 如果有输出，是每秒一次吗？
