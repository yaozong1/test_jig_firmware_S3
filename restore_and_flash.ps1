# 恢复完整固件并烧录
Write-Host "================================" -ForegroundColor Cyan
Write-Host "恢复完整治具固件" -ForegroundColor Cyan
Write-Host "================================" -ForegroundColor Cyan
Write-Host ""

$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
Set-Location $ScriptDir

Write-Host "功能清单：" -ForegroundColor Yellow
Write-Host "  ✓ DUT 控制 (EN/IO0)" -ForegroundColor Green
Write-Host "  ✓ 8通道电压采集" -ForegroundColor Green
Write-Host "  ✓ USB-Serial-JTAG 控制台" -ForegroundColor Green
Write-Host "  ✓ 自动每2秒读取电压" -ForegroundColor Green
Write-Host ""

Write-Host "[1/3] 清理并编译..." -ForegroundColor Green
idf.py fullclean
idf.py build
if ($LASTEXITCODE -ne 0) {
    Write-Host ""
    Write-Host "编译失败！" -ForegroundColor Red
    pause
    exit 1
}

Write-Host ""
Write-Host "[2/3] 烧录到 COM24..." -ForegroundColor Green
idf.py -p COM24 flash
if ($LASTEXITCODE -ne 0) {
    Write-Host ""
    Write-Host "烧录失败！" -ForegroundColor Red
    pause
    exit 1
}

Write-Host ""
Write-Host "[3/3] 打开监视器..." -ForegroundColor Green
Write-Host ""
Write-Host "应该看到：" -ForegroundColor Yellow
Write-Host "  1. 启动信息：Test Jig Firmware - DUT Control + Voltage ADC" -ForegroundColor White
Write-Host "  2. Voltage task created successfully" -ForegroundColor White
Write-Host "  3. >>> Voltage task STARTING <<<" -ForegroundColor White
Write-Host "  4. 每2秒自动读取电压" -ForegroundColor White
Write-Host ""
Write-Host "可用命令：" -ForegroundColor Yellow
Write-Host "  !BOOT    - DUT进入下载模式" -ForegroundColor White
Write-Host "  !RUN     - DUT正常运行" -ForegroundColor White
Write-Host "  !RST     - 复位DUT" -ForegroundColor White
Write-Host "  !VOLTAGE - 手动读取电压" -ForegroundColor White
Write-Host ""
Write-Host "按 Ctrl+] 退出监视器" -ForegroundColor Cyan
Write-Host ""
Start-Sleep -Seconds 2

idf.py -p COM24 monitor
