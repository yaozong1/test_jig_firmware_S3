@echo off
REM 快速重新编译（不完全清理，只重新编译改动的文件）
echo 重新编译 test_jig 固件...
idf.py build
if %ERRORLEVEL% EQU 0 (
    echo.
    echo ========================================
    echo 构建成功！
    echo ========================================
    echo.
    echo 烧录命令：
    echo   idf.py -p COMx flash monitor
    echo.
) else (
    echo.
    echo 构建失败！
)
pause
