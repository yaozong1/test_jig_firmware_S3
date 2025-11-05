# Test Jig Firmware (ESP32-S3)

This is an ESP-IDF project for the factory test jig based on ESP32-S3.

Two roles:
- CDC↔UART Bridge (TinyUSB CDC): PC 上会出现一个“CDC COM 口”，本口桥接到 DUT 的 U0TXD/U0RXD；同时把 CDC 的 DTR/RTS 映射为 DUT 的 EN/IO0，支持 esptool/idf.py 直接对 DUT 烧录与复位。
- （可选）USB-Serial-JTAG：治具自身的日志口，仅用于开发调试，不参与 DUT 烧录。

## Wiring (CDC Bridge)

- DUT U0TXD (GPIO43) -> Jig UART1 RX (默认 GPIO17)
- DUT U0RXD (GPIO44) <- Jig UART1 TX (默认 GPIO18)
- DUT EN             <- Jig GPIO4（由 CDC 的 DTR 控制，断言=低电平）
- DUT IO0            <- Jig GPIO5（由 CDC 的 RTS 控制，断言=低电平）
- GND                <-> GND（共地）

可在 `main/main.c` 中调整 `JIG_UART_RX_PIN` / `JIG_UART_TX_PIN` / `DUT_EN_PIN` / `DUT_IO0_PIN`。

## Build & Flash

1) Ensure ESP-IDF (v5.x) is installed and environment activated.
2) From this folder:

```powershell
idf.py set-target esp32s3
idf.py build
idf.py -p COMx flash monitor
```

Replace `COMx` with the COM port of the jig's USB-Serial-JTAG（用于烧录治具自身）。

启用 TinyUSB CDC（一次性设置）：

1) `idf.py menuconfig`
2) Component config -> TinyUSB：
	- [*] Enable TinyUSB device stack
	- [*] CDC support (CDC-ACM)
3) 保存退出，重新 `idf.py build` 和 `idf.py -p COMx flash`。

PC 端会新增一个 “USB 串行设备 (COMy)”（CDC 口）。该口就是“直连 DUT U0TXD/U0RXD 的桥”，可被 `esptool.py` / `idf.py -p COMy flash` 或你的 Python GUI 用于烧录/交互 DUT。

## Notes

- 烧录 DUT 时，请使用 CDC 口（桥口）；日志口（USB-Serial-JTAG）不要同时占用同一 CDC 口。
- 若 CDC 口下烧录失败，检查 EN/IO0 接线与极性，DTR/RTS 断言语义为“有效低”。
- 速率默认 115200，可按需在 `main.c` 的 `JIG_UART_BAUD` 调整。
