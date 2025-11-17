# CAN Module for Test Jig S3

## 概述

在治具S3中添加了CAN模块,用于向DUT发送周期性测试数据。

## 硬件配置

- **CAN TX引脚**: GPIO40 (ESP32-S3 → CAN收发器TXD → CAN总线)
- **CAN RX引脚**: GPIO39 (ESP32-S3 ← CAN收发器RXD ← CAN总线)
- **波特率**: 250Kbps
- **无需CAN_EN/SHUTDOWN引脚** (根据用户需求)

### 引脚说明
- GPIO40: ESP32-S3的发送引脚，连接到CAN收发器的TXD输入端
- GPIO39: ESP32-S3的接收引脚，连接到CAN收发器的RXD输出端

## 功能特性

1. **自动周期发送**: 每1秒发送一次CAN消息
2. **固定数据**: 01 02 03 04 05 06 07 08
3. **CAN ID**: 0x100 (标准帧)
4. **数据长度**: 8字节

## 实现方式

参考RS485模块的实现,CAN模块包含:

- `can_tx.h` - CAN发送模块头文件
- `can_tx.c` - CAN发送模块实现文件

### 主要函数

```c
bool can_tx_init(void);  // 初始化CAN模块
void can_tx_start(void); // 启动周期发送任务
void can_tx_stop(void);  // 停止发送任务
```

## 集成说明

在 `main.c` 中已经集成:

1. 包含头文件: `#include "can_tx.h"`
2. 在 `app_main()` 中初始化并启动:
   ```c
   if (can_tx_init()) {
       can_tx_start();
       ESP_LOGI(TAG, "CAN TX started");
   }
   ```

## 与RS485的关系

- **RS485模块**: GPIO15(TX) / GPIO16(RX) - 发送 01 02 03 04 05 06 07 08
- **CAN模块**: GPIO39(TX) / GPIO40(RX) - 发送 01 02 03 04 05 06 07 08
- 两个模块**独立运行**,互不干扰

## 日志输出

启动时会看到类似日志:
```
I (xxx) CAN_TX: Initializing CAN module: TX=39, RX=40
I (xxx) CAN_TX: CAN TX init OK: TX=GPIO39, RX=GPIO40, bitrate=250Kbps
I (xxx) CAN_TX: >>> CAN TX task STARTING <<<
I (xxx) CAN_TX: Sending: 01 02 03 04 05 06 07 08 every 1 second
I (xxx) CAN_TX: Sent: 01 02 03 04 05 06 07 08 (ID=0x100)
```

## 编译和烧录

使用现有的构建脚本:
```bash
cd tools/test_jig_firmware
./build.bat
```

## 测试验证

1. **CAN总线分析仪**: 连接到GPIO39/40,观察是否收到周期性消息
2. **示波器**: 测量GPIO39的CAN TX波形
3. **日志检查**: 通过USB查看是否有发送成功日志

## 注意事项

1. CAN总线需要120Ω终端电阻
2. GPIO39和GPIO40需要连接到CAN收发器(如TJA1051)
3. CAN收发器需要外部5V电源供电
4. 如果CAN发送失败,检查日志中的错误信息

## 故障排查

如果CAN不工作:

1. 检查GPIO配置是否正确
2. 检查CAN收发器供电是否正常
3. 检查总线终端电阻
4. 查看日志中的错误信息:
   ```
   E (xxx) CAN_TX: Failed to install TWAI driver: xxx
   E (xxx) CAN_TX: Failed to start TWAI driver: xxx
   W (xxx) CAN_TX: Send failed: xxx
   ```
