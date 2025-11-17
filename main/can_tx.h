// SPDX-License-Identifier: MIT
// CAN TX Module for Test Jig - Send periodic data to DUT

#ifndef CAN_TX_H
#define CAN_TX_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Initialize CAN TX module
// Returns true on success
bool can_tx_init(void);

// Start periodic transmission task (sends 01 02 03 04 05 06 07 08 every 1 second)
void can_tx_start(void);

// Stop periodic transmission
void can_tx_stop(void);

// Trigger 20 message burst (for GUI_REQUEST_DATA)
void can_tx_trigger(void);

#ifdef __cplusplus
}
#endif

#endif // CAN_TX_H
