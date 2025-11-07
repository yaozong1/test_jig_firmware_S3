// Minimal tusb_config.h for TinyUSB on ESP32-S3
#ifndef _TUSB_CONFIG_H_
#define _TUSB_CONFIG_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "sdkconfig.h"

// MCU selection
#ifndef CFG_TUSB_MCU
#define CFG_TUSB_MCU OPT_MCU_ESP32S3
#endif

// Run TinyUSB as device (RHPort0)
#ifndef CFG_TUSB_RHPORT0_MODE
#define CFG_TUSB_RHPORT0_MODE (OPT_MODE_DEVICE | OPT_MODE_FULL_SPEED)
#endif

// OS: FreeRTOS
#ifndef CFG_TUSB_OS
#define CFG_TUSB_OS OPT_OS_FREERTOS
#endif

// Enable device stack
#ifndef CFG_TUD_ENABLED
#define CFG_TUD_ENABLED 1
#endif

// Device mode with one configuration
#ifndef CFG_TUD_ENDPOINT0_SIZE
#define CFG_TUD_ENDPOINT0_SIZE 64
#endif

// Number of CDC interfaces (1 for DUT bridge)
#ifndef CFG_TUD_CDC
#define CFG_TUD_CDC 1
#endif

// CDC FIFO size
#ifndef CFG_TUD_CDC_RX_BUFSIZE
#define CFG_TUD_CDC_RX_BUFSIZE 512
#endif

#ifndef CFG_TUD_CDC_TX_BUFSIZE
#define CFG_TUD_CDC_TX_BUFSIZE 512
#endif

// Debug level (0=none, 1=error, 2=warning, 3=info)
#ifndef CFG_TUSB_DEBUG
#define CFG_TUSB_DEBUG 0
#endif

#ifdef __cplusplus
}
#endif

#endif // _TUSB_CONFIG_H_
