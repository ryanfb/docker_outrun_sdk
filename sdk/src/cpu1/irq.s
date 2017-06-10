/*
	IRQ code for the SECONDARY CPU.
	This only handles a few basic things, everything else should go into the supported user handlers.
	
	- IRQ enable/disable: Functionality to control whether interrupts are enabled from the user code.
	- IRQ waiting:        Wait for a flag to be reset upon interrupt from user code.
	- User Handler:       User function (C) to be invoked upon interrupt. Optional.
	- Clock:              Frame counter. Runs for about 1092 seconds, then wraps. Can be used as 60Hz clock.
*/

/* Note: we need to force this object file into linking, unfortunately, by referencing it from startup.s */

/* From the linker script */
.global __dummy_irq_handler
.global __irq_4_handler

/* User functions */
.global IRQ4_GetCounter
.global IRQ4_Wait
.global IRQ4_SetHandler

.bss

/* Reset when IRQ4 hits (line 223) */
_irq4_wait_ack:
.byte 0

/* Incremented by IRQ4. */
.align 2
_irq4_counter:
.word 0

/* User defined IRQ function */
.align 4
_irq4_userhandler:
.int 0

.text

__irq_4_handler:

	/* Update counters */
	addw #1, _irq4_counter

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
IRQ4_GetCounter:
	move.w _irq4_counter, %D0
	rts

IRQ4_Wait:
	bset #0, _irq4_wait_ack
_IRQ4_wait_loop:
	tst.b    _irq4_wait_ack		/* Wait for it to become zero again */
	bne      _IRQ4_wait_loop
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
