# 快速启动指南

## 立即开始

### 1. 准备环境
```powershell
# 激活 ESP-IDF 环境
C:\Users\<你的用户名>\esp\esp-idf\export.ps1

# 进入项目目录
cd c:\Users\h1576\Desktop\Roam\pe_code\pe_board\tools\test_jig_firmware
```

### 2. 编译固件
```powershell
idf.py build
```

**预期输出**：
```
Project build complete. To flash, run this command:
...
```

### 3. 烧录固件到治具
```powershell
idf.py -p COMx flash
```
将 `COMx` 替换为治具的 USB-Serial-JTAG 端口。

### 4. 连接 DUT

| DUT | 治具 |
|-----|------|
| GPIO43 (U0TXD) | GPIO17 |
| GPIO44 (U0RXD) | GPIO18 |
| EN | GPIO4 |
| IO0 | GPIO5 |
| GND | GND |

### 5. 烧录 DUT
```powershell
# 找到治具的 CDC 端口（不是 USB-Serial-JTAG）
# 在设备管理器中查找新的 COM 端口

# 烧录 DUT
idf.py -p COMy flash
```

## 测试命令

打开串口工具（如 PuTTY、TeraTerm）连接到治具的 CDC 端口，发送：

```
!BOOT   # 进入下载模式
!RUN    # 正常启动
!RST    # 复位
```

## 故障排除

### 找不到 CDC 端口
1. 重新插拔 USB
2. 检查设备管理器
3. 重新烧录治具固件

### 无法烧录 DUT
1. 检查硬件连接
2. 确认使用 CDC 端口（COMy）
3. 尝试手动发送 `!BOOT`

### 编译失败
```powershell
idf.py fullclean
idf.py reconfigure
idf.py build
```

## 完成！

✅ 治具已准备就绪，可以开始测试 DUT！

详细文档请参阅 `README.md`。
