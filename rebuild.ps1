# 清理并重新构建 jig 固件
# 在 ESP-IDF PowerShell 环境中运行（先执行 export.ps1）

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "清理旧的构建文件..." -ForegroundColor Yellow
Write-Host "========================================" -ForegroundColor Cyan

$buildDir = "build"
$managedDir = "managed_components"

if (Test-Path $buildDir) {
    Remove-Item -Recurse -Force $buildDir
    Write-Host "✓ 已删除 build 目录" -ForegroundColor Green
}

if (Test-Path $managedDir) {
    Remove-Item -Recurse -Force $managedDir
    Write-Host "✓ 已删除 managed_components 目录" -ForegroundColor Green
}

Write-Host ""
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "开始重新构建..." -ForegroundColor Yellow
Write-Host "========================================" -ForegroundColor Cyan

# 运行构建
idf.py build

if ($LASTEXITCODE -eq 0) {
    Write-Host ""
    Write-Host "========================================" -ForegroundColor Cyan
    Write-Host "✓ 构建成功！" -ForegroundColor Green
    Write-Host "========================================" -ForegroundColor Cyan
    Write-Host ""
    Write-Host "要烧录到设备，请运行：" -ForegroundColor Yellow
    Write-Host "  idf.py -p COM端口号 flash monitor" -ForegroundColor White
    Write-Host ""
    Write-Host "例如：idf.py -p COM3 flash monitor" -ForegroundColor Gray
    Write-Host "========================================" -ForegroundColor Cyan
} else {
    Write-Host ""
    Write-Host "========================================" -ForegroundColor Cyan
    Write-Host "✗ 构建失败，请检查上面的错误信息" -ForegroundColor Red
    Write-Host "========================================" -ForegroundColor Cyan
}
