# 电压采集模块快速测试指南

## 当前配置

### 自动读取模式
- ✅ **每 2 秒自动读取一次电压**
- ✅ **自动输出到 COM24**
- ✅ **详细的调试信息**

### 硬件连接检查清单

```
治具 S3          K10-3U8 模块
────────────     ──────────────
GPIO17 (TX)  →   RX
GPIO18 (RX)  ←   TX
GND          →   GND
5V (可选)    →   VCC
```

**⚠️ 重要**: TX 和 RX 必须交叉连接！

## 测试步骤

### 1. 编译和烧录
```bash
cd tools\test_jig_firmware
idf.py build
idf.py -p COM24 flash monitor
```

或者使用快捷脚本：
```bash
build_jig.bat
```

### 2. 查看输出

**启动信息**：
```
I (xxx) JIG: ========================================
I (xxx) JIG: Test Jig Firmware - DUT Control + Voltage ADC
I (xxx) JIG: DUT Ctrl : EN=4 IO0=5
I (xxx) JIG: Voltage  : UART1 TX=17 RX=18
I (xxx) JIG: ========================================
I (xxx) VADC: Initializing K10-3U8 voltage module on UART1
I (xxx) VADC: TX=17, RX=18, Baud=9600, Addr=0x01
I (xxx) VADC: Voltage module initialized
```

**每 2 秒的电压读取**：
```
=== Reading Voltage ===
I (xxx) VADC: >>> Sending Modbus RTU request (8 bytes):
I (xxx) VADC:     Addr=0x01 Func=0x03 Start=0x0000 Count=8 CRC=0x....
I (xxx) VADC: 01 03 00 00 00 08 XX XX
I (xxx) VADC:     TX OK: 8 bytes
I (xxx) VADC:     Waiting for response (timeout 500ms)...
I (xxx) VADC:     Received +21 bytes, total: 21
I (xxx) VADC: <<< Received Modbus RTU response (21 bytes):
I (xxx) VADC: 01 03 10 XX XX XX XX XX XX XX XX XX XX XX XX XX XX XX XX XX XX XX
I (xxx) VADC: === Voltage Values ===
I (xxx) VADC:   CH1:  3300 mV (  3.300 V)
I (xxx) VADC:   CH2:  5000 mV (  5.000 V)
I (xxx) VADC:   CH3:     0 mV (  0.000 V)
...
I (xxx) VADC: ======================
VOLTAGE_ADC: {"ch1":3300,"ch2":5000,"ch3":0,...}
Voltage read SUCCESS
======================
```

## 故障诊断

### 情况 1: 无任何响应（超时）
```
<<< No response from module (timeout)
    Check: 1) Wiring (TX<->RX crossed?) 2) Module power 3) Baud rate
```

**解决方案**：
- [ ] 检查 TX/RX 是否交叉连接（S3 TX → 模块 RX）
- [ ] 检查模块是否上电（LED 指示灯）
- [ ] 确认波特率为 9600
- [ ] 检查 GND 连接

### 情况 2: 收到数据但 CRC 错误
```
E (xxx) VADC: CRC mismatch: expected 0x1234, got 0x5678
```

**解决方案**：
- [ ] 检查接线质量
- [ ] 缩短连接线
- [ ] 降低波特率

### 情况 3: 地址不匹配
```
E (xxx) VADC: Address mismatch: expected 0x01, got 0xXX
```

**解决方案**：
- [ ] 检查模块 Modbus 地址（默认 0x01）
- [ ] 修改 `VOLTAGE_MODBUS_ADDR` 宏定义

### 情况 4: 功能码错误或异常
```
E (xxx) VADC: Function code mismatch: expected 0x03, got 0x83
E (xxx) VADC: Modbus exception code: 0xXX
```

**Modbus 异常码**：
- 0x01: 非法功能码
- 0x02: 非法数据地址
- 0x03: 非法数据值
- 0x04: 从机设备故障

## 使用串口助手查看

### 推荐设置
- **端口**: COM24
- **波特率**: 115200
- **数据位**: 8
- **停止位**: 1
- **校验**: None
- **显示**: 文本模式 + 十六进制

### 预期看到
- 启动信息
- 每 2 秒一次完整的读取流程
- Modbus 请求/响应的十六进制数据
- 8 个通道的电压值

## 调试命令

你仍然可以手动发送命令：
- `!VOLTAGE` - 立即读取一次电压
- `!BOOT` - DUT 进入下载模式
- `!RUN` - DUT 运行
- `!RST` - DUT 复位

## 下一步

功能稳定后可以：
1. 修改读取间隔（目前 2 秒）
2. 减少调试输出
3. 添加电压范围检查
4. 集成到工厂测试流程
