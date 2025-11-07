# 治具固件快速编译和烧录脚本
# 使用方法：在 ESP-IDF PowerShell 中运行
# .\build_quick.ps1

Write-Host "================================" -ForegroundColor Cyan
Write-Host "治具固件编译和烧录" -ForegroundColor Cyan
Write-Host "================================" -ForegroundColor Cyan
Write-Host ""

# 检查是否在 ESP-IDF 环境中
if (-not (Get-Command idf.py -ErrorAction SilentlyContinue)) {
    Write-Host "错误: 未找到 idf.py 命令" -ForegroundColor Red
    Write-Host "请先运行: C:\Espressif\frameworks\esp-idf-v5.1.2\export.ps1" -ForegroundColor Yellow
    Write-Host "或者在 ESP-IDF 5.1.2 PowerShell 中运行此脚本" -ForegroundColor Yellow
    exit 1
}

# 切换到脚本所在目录
$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
Set-Location $ScriptDir

Write-Host "[1/3] 编译固件..." -ForegroundColor Green
idf.py build
if ($LASTEXITCODE -ne 0) {
    Write-Host ""
    Write-Host "编译失败！" -ForegroundColor Red
    exit 1
}

Write-Host ""
Write-Host "[2/3] 烧录到 COM24..." -ForegroundColor Green
idf.py -p COM24 flash
if ($LASTEXITCODE -ne 0) {
    Write-Host ""
    Write-Host "烧录失败！检查 COM24 是否正确" -ForegroundColor Red
    exit 1
}

Write-Host ""
Write-Host "[3/3] 打开串口监视器..." -ForegroundColor Green
Write-Host "按 Ctrl+] 退出监视器" -ForegroundColor Yellow
Write-Host ""
Start-Sleep -Seconds 1

idf.py -p COM24 monitor
