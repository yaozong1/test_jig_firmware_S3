@echo off
echo ================================
echo Building and Flashing Jig Firmware
echo ================================

cd /d %~dp0

echo.
echo [1/3] Building...
idf.py build
if errorlevel 1 (
    echo.
    echo ERROR: Build failed!
    pause
    exit /b 1
)

echo.
echo [2/3] Flashing to COM24...
idf.py -p COM24 flash
if errorlevel 1 (
    echo.
    echo ERROR: Flash failed!
    pause
    exit /b 1
)

echo.
echo [3/3] Opening monitor on COM24...
echo Press Ctrl+] to exit monitor
echo.
timeout /t 2
idf.py -p COM24 monitor
