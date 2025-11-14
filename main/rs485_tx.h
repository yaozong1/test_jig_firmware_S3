// SPDX-License-Identifier: MIT
// RS485 TX Module for Test Jig - Send periodic data to DUT

#ifndef RS485_TX_H
#define RS485_TX_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Initialize RS485 TX module
// Returns true on success
bool rs485_tx_init(void);

// Start periodic transmission task (sends 01 02 03 04 05 06 07 08 every 1 second)
void rs485_tx_start(void);

// Stop periodic transmission
void rs485_tx_stop(void);

#ifdef __cplusplus
}
#endif

#endif // RS485_TX_H
