# ESP32-S3 连接诊断脚本
# 用于检查 USB-Serial-JTAG 连接状态

Write-Host "================================" -ForegroundColor Cyan
Write-Host "ESP32-S3 连接诊断" -ForegroundColor Cyan
Write-Host "================================" -ForegroundColor Cyan
Write-Host ""

# 1. 检查串口设备
Write-Host "[1/5] 检查串口设备..." -ForegroundColor Yellow
$ports = [System.IO.Ports.SerialPort]::GetPortNames() | Sort-Object
if ($ports.Count -eq 0) {
    Write-Host "  ❌ 未找到任何串口设备！" -ForegroundColor Red
    Write-Host "     请检查：" -ForegroundColor Yellow
    Write-Host "     - USB 线是否连接" -ForegroundColor Yellow
    Write-Host "     - ESP32-S3 是否上电" -ForegroundColor Yellow
    Write-Host "     - 更换 USB 数据线" -ForegroundColor Yellow
} else {
    Write-Host "  ✓ 找到以下串口：" -ForegroundColor Green
    foreach ($port in $ports) {
        Write-Host "     - $port" -ForegroundColor White
    }
}
Write-Host ""

# 2. 检查设备管理器中的 USB-JTAG 设备
Write-Host "[2/5] 检查 USB-JTAG 设备..." -ForegroundColor Yellow
$usbDevices = Get-PnpDevice | Where-Object { $_.FriendlyName -like "*USB*JTAG*" -or $_.FriendlyName -like "*USB Serial*" }
if ($usbDevices) {
    Write-Host "  ✓ 找到 USB-JTAG 设备：" -ForegroundColor Green
    foreach ($dev in $usbDevices) {
        $status = if ($dev.Status -eq "OK") { "✓" } else { "❌" }
        $color = if ($dev.Status -eq "OK") { "Green" } else { "Red" }
        Write-Host "     $status $($dev.FriendlyName) - 状态: $($dev.Status)" -ForegroundColor $color
    }
} else {
    Write-Host "  ⚠ 未找到 USB-JTAG 设备" -ForegroundColor Yellow
    Write-Host "     可能需要重新插拔 USB" -ForegroundColor Yellow
}
Write-Host ""

# 3. 检查 esptool 是否可用
Write-Host "[3/5] 检查 esptool..." -ForegroundColor Yellow
if (Get-Command esptool.py -ErrorAction SilentlyContinue) {
    Write-Host "  ✓ esptool.py 可用" -ForegroundColor Green
} elseif (Get-Command python -ErrorAction SilentlyContinue) {
    Write-Host "  ⚠ 尝试使用 python -m esptool" -ForegroundColor Yellow
} else {
    Write-Host "  ❌ 未找到 esptool 或 python" -ForegroundColor Red
    Write-Host "     请先激活 ESP-IDF 环境" -ForegroundColor Yellow
}
Write-Host ""

# 4. 尝试连接芯片（如果有端口）
Write-Host "[4/5] 尝试连接芯片..." -ForegroundColor Yellow
if ($ports.Count -gt 0) {
    $testPort = $ports[0]
    Write-Host "  使用端口: $testPort" -ForegroundColor White
    
    if (Get-Command esptool.py -ErrorAction SilentlyContinue) {
        $result = esptool.py --port $testPort chip_id 2>&1
        if ($LASTEXITCODE -eq 0) {
            Write-Host "  ✓ 成功连接到 ESP32-S3！" -ForegroundColor Green
            Write-Host "     端口 $testPort 工作正常" -ForegroundColor Green
        } else {
            Write-Host "  ❌ 无法连接到 $testPort" -ForegroundColor Red
            if ($ports.Count -gt 1) {
                Write-Host "     尝试其他端口..." -ForegroundColor Yellow
                foreach ($p in $ports[1..($ports.Count-1)]) {
                    $result = esptool.py --port $p chip_id 2>&1
                    if ($LASTEXITCODE -eq 0) {
                        Write-Host "  ✓ 成功连接到 $p！" -ForegroundColor Green
                        $testPort = $p
                        break
                    }
                }
            }
        }
    } else {
        Write-Host "  ⚠ 跳过（esptool 不可用）" -ForegroundColor Yellow
    }
} else {
    Write-Host "  ⚠ 跳过（无可用串口）" -ForegroundColor Yellow
}
Write-Host ""

# 5. 检查 sdkconfig
Write-Host "[5/5] 检查 sdkconfig 配置..." -ForegroundColor Yellow
$sdkconfig = "sdkconfig"
if (Test-Path $sdkconfig) {
    $content = Get-Content $sdkconfig
    $consoleConfig = $content | Select-String "CONFIG_ESP_CONSOLE"
    
    if ($consoleConfig -match "CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG=y") {
        Write-Host "  ✓ 控制台配置正确 (USB-Serial-JTAG)" -ForegroundColor Green
    } elseif ($consoleConfig -match "CONFIG_ESP_CONSOLE_UART_DEFAULT=y") {
        Write-Host "  ❌ 控制台配置错误 (UART)" -ForegroundColor Red
        Write-Host "     需要改为 USB-Serial-JTAG" -ForegroundColor Yellow
    } else {
        Write-Host "  ⚠ 控制台配置未知" -ForegroundColor Yellow
    }
} else {
    Write-Host "  ⚠ 未找到 sdkconfig 文件" -ForegroundColor Yellow
}
Write-Host ""

# 总结和建议
Write-Host "================================" -ForegroundColor Cyan
Write-Host "诊断总结" -ForegroundColor Cyan
Write-Host "================================" -ForegroundColor Cyan

if ($ports.Count -eq 0) {
    Write-Host "❌ 严重问题：未检测到任何串口设备" -ForegroundColor Red
    Write-Host ""
    Write-Host "建议操作：" -ForegroundColor Yellow
    Write-Host "1. 检查 USB 线是否插好" -ForegroundColor White
    Write-Host "2. 更换 USB 数据线（有些线只能充电）" -ForegroundColor White
    Write-Host "3. 更换 USB 口（直接插主板，不用 HUB）" -ForegroundColor White
    Write-Host "4. 检查 ESP32-S3 是否上电（LED 灯）" -ForegroundColor White
} elseif ($testPort) {
    Write-Host "✓ 连接正常" -ForegroundColor Green
    Write-Host ""
    Write-Host "使用端口: $testPort" -ForegroundColor Green
    Write-Host ""
    Write-Host "下一步操作：" -ForegroundColor Yellow
    Write-Host "1. 编译固件: idf.py build" -ForegroundColor White
    Write-Host "2. 烧录固件: idf.py -p $testPort flash" -ForegroundColor White
    Write-Host "3. 查看输出: idf.py -p $testPort monitor" -ForegroundColor White
} else {
    Write-Host "⚠ 发现串口但无法连接" -ForegroundColor Yellow
    Write-Host ""
    Write-Host "建议操作：" -ForegroundColor Yellow
    Write-Host "1. 强制进入下载模式（按住 BOOT 按钮 + 按 RESET）" -ForegroundColor White
    Write-Host "2. 关闭所有占用串口的程序" -ForegroundColor White
    Write-Host "3. 重新插拔 USB" -ForegroundColor White
}

Write-Host ""
Write-Host "================================" -ForegroundColor Cyan
