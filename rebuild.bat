@echo off
REM 清理并构建 jig 固件的脚本
REM 请在 ESP-IDF 环境中运行（先执行 export.ps1 或 export.bat）

echo ========================================
echo 清理旧的构建文件...
echo ========================================
if exist build (
    rmdir /s /q build
    echo 已删除 build 目录
)

if exist managed_components (
    rmdir /s /q managed_components
    echo 已删除 managed_components 目录
)

echo.
echo ========================================
echo 开始重新构建...
echo ========================================
idf.py build

if %ERRORLEVEL% EQU 0 (
    echo.
    echo ========================================
    echo ✓ 构建成功！
    echo ========================================
    echo.
    echo 要烧录到设备，请运行：
    echo   idf.py -p COM端口号 flash monitor
    echo.
    echo 例如：idf.py -p COM3 flash monitor
    echo ========================================
) else (
    echo.
    echo ========================================
    echo ✗ 构建失败，请检查错误信息
    echo ========================================
)

pause
