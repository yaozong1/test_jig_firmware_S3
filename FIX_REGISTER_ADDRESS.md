# K10-3U8 寄存器地址修复

## ❌ 原来的错误配置

**寄存器起始地址**: 0x0000  
**结果**: 读到错误的数据（AIN1=5V 时只读到 0x0001）

## ✅ 正确的配置（根据文档）

**寄存器地址范围**: 40033 (0x0020) ~ 40048 (0x002F)

### Modbus 地址计算
- Modbus 寄存器 40001 对应地址 0x0000
- 寄存器 40033 对应地址 **0x0020** ✅
- 因此读取 AI1-AI8 应该从 **0x0020** 开始

### 数据格式
- **量程**: 0~10V
- **数字范围**: 0~32768 (0x0000~0x7FFF)
- **换算公式**: 电压(mV) = 原始值 × 10000 / 32768

### 示例验证
当 AI1 = 5V 时：
- **原始值**: 应该约为 16384 (0x4000)
  - 计算：5V / 10V × 32768 = 16384
- **换算**: 16384 × 10000 / 32768 = 5000 mV ✅

## 🔧 已修改的内容

### 1. voltage_adc.c
```c
// 修改前
tx_buf[3] = 0x00;  // Start address low byte (0x0000) ❌

// 修改后
tx_buf[3] = 0x20;  // Start address low byte (0x0020) ✅
```

### 2. 数据换算
```c
// 修改前
voltages[i] = raw;  // 直接使用原始值 ❌

// 修改后
uint32_t voltage_mv = ((uint32_t)raw * 10000) / 32768;  // 正确换算 ✅
voltages[i] = (uint16_t)voltage_mv;
```

### 3. 文档更新
- VOLTAGE_ADC_README.md: 更新寄存器地址和换算公式
- DEBUG_K10_DATA_FORMAT.md: 记录调试过程

## 📊 预期结果

重新编译烧录后，当 AIN1 = 5V 时应该看到：

```
I (xxx) VADC: >>> Sending Modbus RTU request (8 bytes):
I (xxx) VADC:     Addr=0x01 Func=0x03 Start=0x0020 Count=8 CRC=0x45C6
I (xxx) VADC: 01 03 00 20 00 08 45 C6
I (xxx) VADC:     TX OK: 8 bytes
I (xxx) VADC:     Waiting for response (timeout 500ms)...
I (xxx) VADC:     Received +21 bytes, total: 21
I (xxx) VADC: <<< Received Modbus RTU response (21 bytes):
I (xxx) VADC: 01 03 10 40 00 00 03 ...  ← CH1 原始值 0x4000 (16384)
I (xxx) VADC: === Voltage Values ===
I (xxx) VADC:   CH1: Raw=16384 (0x4000) -> 5000 mV (  5.000 V)  ← 正确！
I (xxx) VADC:   CH2: Raw=    3 (0x0003) ->    0 mV (  0.000 V)
...
VOLTAGE_ADC: {"ch1":5000,"ch2":0,...}
```

## 🚀 立即测试

```powershell
cd C:\Users\h1576\Desktop\Roam\pe_code\pe_board\tools\test_jig_firmware
idf.py build
idf.py -p COM24 flash monitor
```

## 📝 参考文档

根据 K10-3U8 用户手册 4.3.2 节：
- 过程量采集：将 0~10V 转换成 0~32768
- 支持功能码：0x03, 0x04
- 地址范围：40033 (0x0020) ~ 40048 (0x002F)

**示例**（从手册）：
- 当 AI1=5V, AI2=0V, AI3=5V, AI4=0V
- 发送：01 03 00 20 00 08 45 C6
- 接收：01 03 10 7F FF 00 00 7F FF 00 00 00 00 00 00 00 00 D1 C3
  - AI1 = 0x7FFF (32767) ≈ 10V (满量程)
  - AI3 = 0x7FFF (32767) ≈ 10V
