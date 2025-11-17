# CAN模块快速测试指南

## 1. 编译固件

```powershell
cd c:\Users\h1576\Desktop\Roam\pe_code\pe_board\tools\test_jig_firmware
.\build.bat
```

## 2. 烧录固件

```powershell
.\build_and_flash.bat
```

或者手动烧录:
```powershell
cd build
esptool.py -p COM<端口号> write_flash @flash_args
```

## 3. 查看日志

使用串口工具(115200波特率)或ESP-IDF监视器:
```powershell
idf.py monitor
```

## 4. 预期日志输出

启动后应该看到:
```
I (xxx) JIG: ========================================
I (xxx) JIG: Test Jig Firmware - DUT Control + Voltage ADC
I (xxx) JIG: Initializing CAN TX module...
I (xxx) CAN_TX: Initializing CAN module: TX=39, RX=40
I (xxx) CAN_TX: CAN TX init OK: TX=GPIO39, RX=GPIO40, bitrate=250Kbps
I (xxx) JIG: CAN TX started
I (xxx) CAN_TX: >>> CAN TX task STARTING <<<
I (xxx) CAN_TX: Sending: 01 02 03 04 05 06 07 08 every 1 second
I (xxx) CAN_TX: Sent: 01 02 03 04 05 06 07 08 (ID=0x100)
I (xxx) CAN_TX: Sent: 01 02 03 04 05 06 07 08 (ID=0x100)
...
```

## 5. 硬件连接验证

### 最简单的测试方法 - 使用CAN分析仪

1. 连接CAN分析仪到GPIO39/40
2. 设置波特率为250Kbps
3. 观察是否每秒收到一次消息:
   - ID: 0x100
   - 数据: 01 02 03 04 05 06 07 08

### 使用示波器验证

1. 探头连接到GPIO39 (CAN TX)
2. 应该看到每秒一次的CAN波形
3. 波形应该是差分信号

### 回环测试(如果有两块板子)

1. 板子A的CAN TX(GPIO39) → 板子B的CAN RX(GPIO40)
2. 板子B的CAN TX(GPIO39) → 板子A的CAN RX(GPIO40)
3. 共地连接
4. 添加120Ω终端电阻

## 6. 故障排查

### CAN初始化失败
```
E (xxx) CAN_TX: Failed to install TWAI driver: xxx
```
**可能原因**:
- GPIO引脚冲突
- TWAI驱动已被其他模块使用
- sdkconfig配置问题

**解决方法**:
- 检查GPIO39/40是否被其他模块占用
- 重新配置sdkconfig: `idf.py menuconfig`

### CAN发送失败
```
W (xxx) CAN_TX: Send failed: ESP_ERR_INVALID_STATE
```
**可能原因**:
- CAN总线未连接(BUS_OFF状态)
- 没有终端电阻
- 总线负载过重

**解决方法**:
- 连接CAN收发器和总线
- 添加120Ω终端电阻
- 检查总线是否短路

### 无日志输出
**可能原因**:
- 串口波特率不匹配
- USB连接问题
- 固件未正确烧录

**解决方法**:
- 确认波特率115200
- 重新插拔USB
- 重新烧录固件

## 7. CAN收发器硬件参考

### TJA1051/TJA1051T推荐电路

```
ESP32-S3 GPIO40 (CAN TX) ──→ TJA1051 TXD (收发器输入)
ESP32-S3 GPIO39 (CAN RX) ←── TJA1051 RXD (收发器输出)
                             TJA1051 VCC  ── 5V
                             TJA1051 GND  ── GND
                             TJA1051 CANH ─┬─ 120Ω ─┬─ CAN BUS
                             TJA1051 CANL ─┘        └─ CAN BUS
```

**引脚连接说明**:
- GPIO40 (TX): ESP32-S3发送数据 → TJA1051的TXD引脚 → 转换到CAN总线
- GPIO39 (RX): CAN总线 → TJA1051的RXD引脚 → ESP32-S3接收数据

### 注意事项
- TJA1051需要5V供电
- GPIO电平为3.3V,TJA1051兼容
- 总线两端各需要一个120Ω终端电阻
- 最大总线长度取决于波特率(250Kbps约250米)

## 8. 与RS485模块对比

| 特性 | RS485模块 | CAN模块 |
|------|----------|---------|  
| 引脚 | GPIO15/16 | GPIO40(TX)/39(RX) |
| 协议 | UART RS485 | CAN 2.0 |
| 波特率 | 115200 | 250Kbps |
| 数据 | 01 02 03 04 05 06 07 08 | 01 02 03 04 05 06 07 08 |
| 间隔 | 1秒 | 1秒 |
| 状态 | 已运行 | 新添加 |

两个模块互不干扰,可以同时运行。
