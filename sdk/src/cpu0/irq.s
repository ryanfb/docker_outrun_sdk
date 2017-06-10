/*
	IRQ code for the MAIN CPU.
	This only handles a few basic things, everything else should go into the supported user handlers.
	
	- IRQ enable/disable: Functionality to control whether interrupts are enabled from the user code.
	- IRQ waiting:        Wait for a flag to be reset upon interrupt from user code.
	- User Handler:       User function (C) to be invoked upon interrupt. Optional.
	- Watchdog Reset:     Handles automatic watchdog reset once a frame, when enabled.
	- Clock:              Frame counter. Runs for about 1092 seconds, then wraps. Can be used as 60Hz clock.
*/

/* Note: we need to force this object file into linking, unfortunately, by referencing it from startup.s */

/* From the linker script */
.global __dummy_irq_handler
.global __irq_2_handler
.global __irq_4_handler

/* User functions */
.global IRQ2_GetCounter
.global IRQ2_Wait
.global IRQ4_GetCounter
.global IRQ4_Wait
.global IRQ_WaitAny
.global IRQ2_SetHandler
.global IRQ4_SetHandler

.bss

/* Reset when IRQ2 hits (lines 65, 129, 193) */
_irq2_wait_ack:
.byte 0

/* Reset when IRQ4 hits (line 223) */
_irq4_wait_ack:
.byte 0

/* Reset by IRQ4, incremented by IRQ2. */
_irq2_counter:
.byte 0

/* Incremented by IRQ4. */
.align 2
_irq4_counter:
.word 0

/* User defined IRQ functions */
.align 4
_irq2_userhandler:
.int 0
_irq4_userhandler:
.int 0

.text

__irq_2_handler:

	/* Update counters: Increment IRQ 2 counter (0..1..2..3) */
	addb #1, _irq2_counter

	/* Reset wait */
	clr.b    _irq2_wait_ack
	
	/* Wait out the rest of the scanline so the interrupt doesn't trigger twice.
	   we have 262 scanlines @ 60fps = 15720 scanlines per second. So one scanline is 654,8 cycles @ 10Mhz.
	   Since dbra takes at least 10 cycles, we can loop 66x to waste at least 660 cycles */
	move.l   %D0, -(%A7)
	clr.l    %D0
	move.b   #66, %D0
_irq2_wait:
	dbra     %D0, _irq2_wait
	move.l   (%A7)+, %D0
	
	/* User function */
	tst.l    _irq2_userhandler
	beq      _irq2_done
	
	/* Store registers on the stack */
	movem.l %D0-%D7/%A0-%A6, -(%A7)

	/* Run user handlers */
	/* Todo: to user mode */
	move.l _irq2_userhandler, %A0
	jsr (%A0)

	/* Restore registers from the stack */
	movem.l (%A7)+, %D0-%D7/%A0-%A6

_irq2_done:
	rte
	
__irq_4_handler:

	/* Watchdog clear, could be optional */
	tst.w 0x140060.l 

	/* Update counters */
	clr.b    _irq2_counter
	addw #1, _irq4_counter /* dit blaast op */

	/* Reset wait */
	clr.b    _irq4_wait_ack
	
	/* Wait out the rest of the scanline so the interrupt doesn't trigger twice.
	   we have 262 scanlines @ 60fps = 15720 scanlines per second. So one scanline is 654,8 cycles @ 10Mhz.
	   Since dbra takes at least 10 cycles, we can loop 66x to waste at least 660 cycles */
	move.l   %D0, -(%A7)
	clr.l    %D0
	move.b   #66, %D0
_irq4_wait:
	dbra     %D0, _irq4_wait
	/* We are now at scanline 224 (invisible) */
	move.l   (%A7)+, %D0

	/* Check for user function */
	
	/* Store registers on the stack */
	/*movem.l %D0-%D7/%A0-%A6, -(%A7) */

	/* Run user handlers */
	/* ... */

	/* Restore registers from the stack */
	/* movem.l (%A7)+, %D0-%D7/%A0-%A6 */

	rte

__dummy_irq_handler:
	rte

	/* User functions */
IRQ2_GetCounter:
	move.b _irq2_counter, %D0
	rts

IRQ4_GetCounter:
	move.w _irq4_counter, %D0
	rts

IRQ2_Wait:
	bset #0, _irq2_wait_ack
_IRQ2_wait_loop:
	tst.b    _irq2_wait_ack		/* Wait for it to become zero again */
	bne      _IRQ2_wait_loop
	move.b   _irq2_counter, %D0	/* Report the current section */
    rts

IRQ4_Wait:
	bset #0, _irq4_wait_ack
_IRQ4_wait_loop:
	tst.b    _irq4_wait_ack		/* Wait for it to become zero again */
	bne      _IRQ4_wait_loop
    rts

	/* Wait for any of the two */
IRQ_WaitAny:
	/* Set both bits */
	bset #0, _irq2_wait_ack
	bset #0, _irq4_wait_ack
	/* And wait for either of them to become zero */
_IRQany_wait_loop:
	tst.b    _irq2_wait_ack		/* Wait for it to become zero again */
	beq _IRQany_done
	tst.b    _irq4_wait_ack		/* Wait for it to become zero again */
	bne      _IRQany_wait_loop
_IRQany_done:
	move.b   _irq2_counter, %D0	/* Report the current section */
    rts

IRQ2_SetHandler:
	/* Save old IRQ flags on the stack; to do this just store the status register */
	move.w %SR, -(%SP)

	/* Disable all interrupts for now */
	moveq #7, %D0 
	trap #0
	
	/* Set vector (argument on stack) */
	move.l (0x6, %A7), (_irq2_userhandler)
	
	/* Restore previous interrupt state */
	move.w (%SP)+, %D0
	lsr.w #8, %D0
	and.w #7, %D0
	trap #0
	rts
	
IRQ4_SetHandler:
	/* Save old IRQ flags on the stack; to do this just store the status register */
	move.w %SR, -(%SP)

	/* Disable all interrupts for now */
	moveq #7, %D0 
	trap #0
	
	/* Set vector (argument on stack) */
	move.l (0x6, %A7), (_irq4_userhandler)
	
	/* Restore previous interrupt state */
	move.w (%SP)+, %D0
	lsr.w #8, %D0
	and.w #7, %D0
	trap #0
	rts
