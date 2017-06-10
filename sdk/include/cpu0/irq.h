#ifndef __IRQ_H__
#define __IRQ_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" 
{
#endif // __cplusplus

// Retrieves the current IRQ 2 count, or waits for the next interrupt and returns the count.
// 0: Line 223..64 (not returned by IRQ2_Wait)
// 1: Line 65..128
// 2: Line 129..192
// 3: Line 193..222
extern uint8_t IRQ2_GetCounter ();
extern uint8_t IRQ2_Wait ();

// Retrieves IRQ4 counter. Fired once per frame, at scanline 223. Basically a 16-bit frame count.
extern uint16_t IRQ4_GetCounter ();

// Wait for the next IRQ4 to occur.
extern void IRQ4_Wait ();

// Wait for any of the above.
extern uint8_t IRQ_WaitAny ();

// User defined IRQ handlers.
// These are executed in user mode.
typedef void IRQFUNC (void);
extern void IRQ2_SetHandler (IRQFUNC* pFunc);
extern void IRQ4_SetHandler (IRQFUNC* pFunc);

#ifdef __cplusplus
} // extern "C"
#endif // __cplusplus

#endif // __IRQ_H__
