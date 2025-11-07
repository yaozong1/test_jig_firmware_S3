# 验证 Voltage Task 是否运行

## 问题诊断

如果你发送 `!VOLTAGE` 有响应，但没有看到自动的电压输出，可能的原因：

### 1. 固件没有重新烧录
❌ 修改了代码但没有重新编译烧录
✅ **解决方案**: 必须重新编译和烧录

### 2. Voltage Task 创建失败
❌ 内存不足或任务创建失败
✅ **解决方案**: 查看启动日志，应该看到 "Voltage task created successfully"

### 3. Voltage Task 被阻塞
❌ voltage_adc_init() 卡住
❌ UART 初始化失败
✅ **解决方案**: 查看启动日志中的详细错误信息

## 验证步骤

### 步骤 1: 重新编译和烧录

**在 ESP-IDF 5.1.2 PowerShell 中运行**：

```powershell
cd C:\Users\h1576\Desktop\Roam\pe_code\pe_board\tools\test_jig_firmware
.\build_quick.ps1
```

或者手动：

```powershell
idf.py build
idf.py -p COM24 flash monitor
```

### 步骤 2: 检查启动日志

烧录后，应该看到以下信息（按顺序）：

```
========================================
Test Jig Firmware - DUT Control + Voltage ADC
DUT Ctrl : EN=4 IO0=5
Voltage  : UART1 TX=17 RX=18
========================================
I (xxx) JIG: Initializing voltage ADC...
I (xxx) VADC: Initializing K10-3U8 voltage module on UART1
I (xxx) VADC: TX=17, RX=18, Baud=9600, Addr=0x01
I (xxx) VADC: Voltage module initialized
I (xxx) JIG: Voltage ADC initialized
I (xxx) JIG: Creating console task...
I (xxx) JIG: Creating voltage task...
I (xxx) JIG: Voltage task created successfully    ← 关键！
I (xxx) JIG: Control running. Commands: !BOOT !RUN !RST !VOLTAGE
I (xxx) JIG: Voltage auto-reading enabled (every 2 seconds)
I (xxx) JIG: ========================================
I (xxx) JIG: >>> Voltage task STARTING <<<         ← 关键！

>>> Voltage task started, reading every 2 seconds... <<<
```

**2 秒后应该看到**：

```
=== Reading Voltage [1] ===
I (xxx) JIG: Voltage read attempt 1
I (xxx) VADC: >>> Sending Modbus RTU request (8 bytes):
I (xxx) VADC:     Addr=0x01 Func=0x03 Start=0x0000 Count=8 CRC=0x...
I (xxx) VADC: 01 03 00 00 00 08 XX XX
I (xxx) VADC:     TX OK: 8 bytes
I (xxx) VADC:     Waiting for response (timeout 500ms)...
```

### 步骤 3: 判断问题

#### 情况 A: 看到 "Voltage task created successfully" 但没有 ">>> Voltage task STARTING <<<"
**问题**: Voltage task 创建了但没有运行
**可能原因**: 
- 任务优先级被其他任务抢占
- 内存不足
- 调度器问题

**解决方案**:
```c
// 增加 voltage task 优先级（在 main.c 中）
xTaskCreate(voltage_task, "voltage", 8192, NULL, 6, NULL);  // 改为 8192 堆栈，优先级 6
```

#### 情况 B: 看到 "FAILED to create voltage task!"
**问题**: 任务创建失败
**原因**: 内存不足

**解决方案**:
```c
// 减小其他任务堆栈或降低 voltage task 堆栈
xTaskCreate(voltage_task, "voltage", 2048, NULL, 4, NULL);  // 试试 2048
```

#### 情况 C: 看到 ">>> Voltage task STARTING <<<" 但卡住了
**问题**: voltage_adc_read_all() 阻塞或卡死
**原因**: UART 初始化问题

**解决方案**: 检查 voltage_adc.c 中的 UART 配置

#### 情况 D: 什么都看不到
**问题**: 固件没有烧录成功或串口不对
**解决方案**: 
1. 确认 COM24 端口正确
2. 重新插拔 USB
3. 检查设备管理器

### 步骤 4: 使用串口助手验证

如果 `idf.py monitor` 有问题，使用第三方串口助手：

- 端口: COM24
- 波特率: 115200
- 数据位: 8
- 停止位: 1
- 校验: 无

**复位 S3** (拔插 USB 或按复位键)，应该看到完整的启动日志。

## 快速测试命令

烧录后，发送以下命令测试：

```
!RST        # 复位，观察启动日志
!VOLTAGE    # 手动读取电压
!RUN        # 复位 DUT
```

## 当前代码增强

已添加以下调试信息：

1. ✅ Voltage task 创建结果检查
2. ✅ Voltage task 启动日志
3. ✅ 每次读取带计数器 [1], [2], [3]...
4. ✅ 详细的 ESP_LOGI/LOGE 日志
5. ✅ 增加初始等待时间到 2 秒

## 预期输出频率

如果一切正常：
- **每 2 秒**一次完整的电压读取日志
- 包括 Modbus 请求、响应、解析结果
- 如果模块未连接，会看到超时错误

## 下一步

1. **立即执行**: 运行 `.\build_quick.ps1` 重新烧录
2. **观察启动**: 查看所有关键日志是否出现
3. **等待 2 秒**: 观察是否有自动读取
4. **报告结果**: 告诉我你看到了什么
