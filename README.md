# Test Jig Firmware (ESP32-S3)

This is a minimal ESP-IDF project for the factory test jig based on ESP32-S3.
It forwards the DUT's UART logs to the USB-Serial-JTAG (the COM port visible on PC), so the Python GUI can parse "SELFTEST SUMMARY: { ... }" JSON directly.

## Wiring (minimal)

- DUT TX -> Jig RX (default GPIO17)
- DUT RX -> Jig TX (default GPIO18) [optional for now]
- GND     -> GND (common ground)

Adjust pins in `main/main.c` macros `JIG_UART_RX_PIN` / `JIG_UART_TX_PIN` if needed.

## Build & Flash

1) Ensure ESP-IDF (v5.x) is installed and environment activated.
2) From this folder:

```powershell
idf.py set-target esp32s3
idf.py build
idf.py -p COMx flash monitor
```

Replace `COMx` with the COM port of the jig's USB-Serial-JTAG.

After flashing, open the same COM port in the Python GUI and you should see DUT outputs.

## Next Steps (Roadmap)

- RS485 Echo: add MAX485 and a UART task to echo back payloads (01..08) so DUT RS485 selftest passes.
- CAN Echo: add TJA1051 transceiver and a TWAI task to echo frames (ID=0x321, data=01..08).
- 8-Channel Voltage: integrate external ADCs (ADS1115 x2 or MCP3208) and periodically send voltages as JSON to GUI.
- Flash DUT from Jig: map USB DTR/RTS to DUT EN/IO0, or implement a small CDC<->UART bridge to allow `idf.py` flashing through the jig.
