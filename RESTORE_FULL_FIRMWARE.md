# 完整治具固件烧录步骤

## ✅ 硬件已验证正常
- USB-Serial-JTAG 工作正常
- COM24 输出正常
- 现在恢复完整功能

## 🚀 立即执行（在 ESP-IDF PowerShell 中）

### 步骤 1: 切换目录
```powershell
cd C:\Users\h1576\Desktop\Roam\pe_code\pe_board\tools\test_jig_firmware
```

### 步骤 2: 清理并重新编译
```powershell
idf.py fullclean
idf.py build
```

### 步骤 3: 烧录
```powershell
idf.py -p COM24 flash
```

### 步骤 4: 查看输出
```powershell
idf.py -p COM24 monitor
```

**或者一条命令完成：**
```powershell
idf.py fullclean && idf.py -p COM24 flash monitor
```

## 📊 预期输出

### 启动信息（立即显示）
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
I (xxx) JIG: Voltage task created successfully    ← 关键
I (xxx) JIG: Control running. Commands: !BOOT !RUN !RST !VOLTAGE
I (xxx) JIG: Voltage auto-reading enabled (every 2 seconds)
I (xxx) JIG: ========================================
I (xxx) JIG: >>> Voltage task STARTING <<<         ← 关键

>>> Voltage task started, reading every 2 seconds... <<<
```

### 2秒后（自动重复）
```
=== Reading Voltage [1] ===
I (xxx) JIG: Voltage read attempt 1
I (xxx) VADC: >>> Sending Modbus RTU request (8 bytes):
I (xxx) VADC:     Addr=0x01 Func=0x03 Start=0x0000 Count=8 CRC=0x...
I (xxx) VADC: 01 03 00 00 00 08 XX XX
I (xxx) VADC:     TX OK: 8 bytes
I (xxx) VADC:     Waiting for response (timeout 500ms)...
```

**如果电压模块已连接**：
```
I (xxx) VADC:     Received +21 bytes, total: 21
I (xxx) VADC: <<< Received Modbus RTU response (21 bytes):
I (xxx) VADC: 01 03 10 XX XX XX XX ...
I (xxx) VADC: === Voltage Values ===
I (xxx) VADC:   CH1:  3300 mV (  3.300 V)
I (xxx) VADC:   CH2:  5000 mV (  5.000 V)
...
VOLTAGE_ADC: {"ch1":3300,"ch2":5000,...}
Voltage read SUCCESS
```

**如果电压模块未连接**：
```
I (xxx) VADC: <<< No response from module (timeout)
I (xxx) VADC:     Check: 1) Wiring (TX<->RX crossed?) 2) Module power 3) Baud rate
E (xxx) JIG: Voltage read FAILED
Voltage read FAILED
```

## 🎮 可用命令

烧录后，可以在串口监视器中发送：

- **`!BOOT`** - 让 DUT 进入下载模式
  ```
  JIG: BOOT start
  JIG: BOOT OK (DUT in bootloader mode)
  ```

- **`!RUN`** - 让 DUT 正常运行
  ```
  JIG: RUN start
  JIG: RUN OK (DUT running)
  ```

- **`!RST`** - 复位 DUT
  ```
  JIG: RST
  ```

- **`!VOLTAGE`** - 手动读取一次电压
  ```
  JIG: Reading 8-channel voltage...
  (显示电压值)
  JIG: Voltage read OK
  ```

## 🔧 功能说明

### 1. 自动电压监控
- ✅ 固件启动后自动运行
- ✅ 每 2 秒读取一次
- ✅ 无需手动发送命令
- ✅ 持续输出到串口

### 2. DUT 控制
- ✅ GPIO4 控制 DUT EN（复位）
- ✅ GPIO5 控制 DUT IO0（启动模式）
- ✅ 支持进入下载模式/运行模式

### 3. 电压采集
- ✅ K10-3U8 模块通过 UART1 (GPIO17/18)
- ✅ Modbus RTU 协议，9600 波特率
- ✅ 8 个通道电压值
- ✅ JSON 格式输出（供 GUI 解析）

## 📝 下一步集成到 GUI

完整固件运行正常后，需要修改 GUI 来：

1. **解析 VOLTAGE_ADC JSON**
   ```python
   # 从 COM24 读取的数据中匹配
   if "VOLTAGE_ADC:" in line:
       json_str = line.split("VOLTAGE_ADC:")[1].strip()
       data = json.loads(json_str)
       # 更新 GUI 中的电压表格
   ```

2. **显示电压值**
   - 8 个通道的电压表格
   - 实时更新（每 2 秒）
   - 可选：添加电压范围检查

3. **保存到测试报告**
   - 将电压值加入 SELFTEST SUMMARY

## ⚠️ 故障排除

### 如果看到 "Voltage read FAILED"
**原因**：电压模块未连接或接线错误

**检查**：
1. GPIO17 (S3 TX) → 模块 RX
2. GPIO18 (S3 RX) ← 模块 TX
3. GND 连接
4. 模块供电 (5V/3.3V)

### 如果没有自动读取
**检查**：
1. 是否看到 "Voltage task created successfully"
2. 是否看到 ">>> Voltage task STARTING <<<"
3. 如果没有，说明任务创建失败（内存不足？）

### 如果读取成功但数据异常
**检查**：
1. CRC 校验是否通过
2. 电压值是否合理（0-30000 mV）
3. 模块地址是否为 0x01

## 🎯 成功标志

固件工作正常的标志：
- ✅ 启动时看到所有初始化信息
- ✅ 看到 "Voltage task created successfully"
- ✅ 看到 ">>> Voltage task STARTING <<<"
- ✅ 每 2 秒自动输出一次读取结果
- ✅ 可以手动发送 !VOLTAGE 命令
- ✅ DUT 控制命令响应正常
