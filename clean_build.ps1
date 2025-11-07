# 强制完全重新构建
Write-Host "====================" -ForegroundColor Cyan
Write-Host "清理构建目录..." -ForegroundColor Yellow
Write-Host "====================" -ForegroundColor Cyan

if (Test-Path "build") {
    Remove-Item -Recurse -Force "build"
    Write-Host "✓ 已删除 build 目录" -ForegroundColor Green
}

Write-Host ""
Write-Host "====================" -ForegroundColor Cyan  
Write-Host "开始构建..." -ForegroundColor Yellow
Write-Host "====================" -ForegroundColor Cyan

idf.py build

if ($LASTEXITCODE -eq 0) {
    Write-Host ""
    Write-Host "====================" -ForegroundColor Green
    Write-Host "✓ 构建成功！" -ForegroundColor Green
    Write-Host "====================" -ForegroundColor Green
} else {
    Write-Host ""
    Write-Host "====================" -ForegroundColor Red
    Write-Host "✗ 构建失败" -ForegroundColor Red  
    Write-Host "====================" -ForegroundColor Red
}
