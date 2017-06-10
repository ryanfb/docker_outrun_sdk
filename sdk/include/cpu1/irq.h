#ifndef __IRQ_H__
#define __IRQ_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" 
{
#endif // __cplusplus

// Retrieves IRQ4 counter. Fired once per frame, at scanline 223. Basically a 16-bit frame count.
extern uint16_t IRQ4_GetCounter ();

// Wait for the next IRQ4 to occur.
extern void IRQ4_Wait ();

// User defined IRQ handler.
// This should be executed in user mode.
typedef void IRQFUNC (void);
extern void IRQ4_SetHandler (IRQFUNC* pFunc);

#ifdef __cplusplus
} // extern "C"
#endif // __cplusplus

#endif // __IRQ_H__
