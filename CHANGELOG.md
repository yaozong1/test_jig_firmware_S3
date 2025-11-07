# 治具固件更新说明

## 修改内容

### 1. 代码清理 (main.c)
✅ 删除所有关于 CH340/UART0 直连 DUT 的引用
✅ 更新启动信息和命令响应消息
✅ 保留核心功能：
  - DUT 控制 (EN/IO0)
  - 电压采集 (UART1)
  - USB-Serial-JTAG 控制台

### 2. 配置修改 (sdkconfig)
✅ 主控制台：UART0 → USB-Serial-JTAG
✅ 次要控制台：USB-Serial-JTAG → 无
✅ 结果：所有日志输出到 COM24

### 3. 文档更新
✅ FLASH_GUIDE.md - 删除 CH340 引用
✅ VOLTAGE_TEST_GUIDE.md - 更新启动信息
✅ README_NEW.md - 创建清晰的新文档

## 当前架构

```
┌─────────────┐
│     PC      │
└──────┬──────┘
       │ USB
       │
┌──────▼──────────────────┐
│  治具 ESP32-S3         │
│                         │
│  USB-Serial-JTAG       │ ← 所有日志和控制
│  (COM24)                │
│                         │
│  GPIO4  → DUT EN       │ ← DUT 复位
│  GPIO5  → DUT IO0      │ ← DUT 启动模式
│                         │
│  UART1 (GPIO17/18)     │ ← Modbus RTU
│  ↓↑                     │
│  K10-3U8 电压模块      │ ← 8通道电压采集
└─────────────────────────┘
```

## 功能对比

### 之前（CDC 桥接方案）
- ❌ CH340 直连 DUT UART0
- ❌ 治具作为 USB-CDC 桥
- ❌ 复杂的透传逻辑
- ✅ USB-Serial-JTAG 用于治具日志

### 现在（简化方案）
- ✅ 仅保留 DUT 控制 (EN/IO0)
- ✅ 电压采集通过 UART1
- ✅ USB-Serial-JTAG 作为主控制台
- ✅ 更简洁的架构

## 下一步

1. **编译并烧录**：
   ```cmd
   cd tools\test_jig_firmware
   idf.py build
   idf.py -p COM24 flash monitor
   ```

2. **验证输出**：
   - 应该在 COM24 看到启动信息
   - 每 2 秒看到电压读取尝试
   - Modbus 通信详细日志

3. **测试功能**：
   - 发送 `!BOOT` / `!RUN` / `!RST` 命令
   - 观察电压采集是否正常
   - 检查 JSON 输出格式

## 保留的旧文档

以下文档描述旧架构，可以删除或归档：
- `README_DIRECT.md` - CDC 桥接方案说明
- `README.md` - 原始文档（内容混乱）

建议：
- 删除 `README.md`
- 重命名 `README_NEW.md` → `README.md`
- 归档 `README_DIRECT.md` 到历史目录
