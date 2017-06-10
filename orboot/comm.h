#ifndef __COMM_H__
#define __COMM_H__

#include <Platform.h>

// Parallel protocol based on IEEE1284.
// Difference is that we actually wait for the transitions instead of relying on
// adequate 500ns response time. We don't have hooked up interrupts and 10MHz is too slow.

// Initializes timers. Run from the same thread as all the other calls.
void COMM_Init ();

// Set the transition delay. Default is 50ms to be safe. 35ms is about the minimum transition time
// for the TLP521 optocouplers used on the Outrun board.
void COMM_SetTXDelayNS (uint64 nanoseconds);

// Sets a debug delay for all output transitions. Zero (default) means no delay.
void COMM_SetDebugDelayMS (uint32 milliseconds);

// Resets the control register:
// - nSTROBE line is low (we inverted this ourselves!) (not strobing).
// - nAUTOFEED line is high (HostBusy for nibble mode).
// - Data bits are cleared.
void COMM_Reset ();

// Sets the response timeout. Set to zero for no timeout.
void COMM_SetRXTimeOutMS (uint32 milliseconds);
void COMM_SetTXTimeOutMS (uint32 milliseconds);

// Sets an inversion mask for the control register. Default is zero (normal behavior).
void COMM_SetControlInversionMask (uint8 mask);

// Sends a single byte in 'compatibility mode'.
// Returns false if the send times out.
bool COMM_SendByte (uint8 byte);

// Reads a single byte in 'nibble' mode.
// Returns -1 if the receive times out.
int16 COMM_RecvByte ();

#endif // __COMM_H__
