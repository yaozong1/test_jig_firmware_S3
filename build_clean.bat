@echo off
echo ========================================
echo 开始构建 test_jig 固件
echo ========================================
echo.

cd /d %~dp0

if exist build (
    echo 删除旧的 build 目录...
    rmdir /s /q build
)

echo.
echo 运行 idf.py build...
echo.

idf.py build

echo.
if %ERRORLEVEL% EQU 0 (
    echo ========================================
    echo 构建成功！
    echo ========================================
) else (
    echo ========================================
    echo 构建失败，错误代码: %ERRORLEVEL%
    echo ========================================
)

pause
