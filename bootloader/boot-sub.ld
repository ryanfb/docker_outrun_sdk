/* 
	Sega Out Run bootloader linker script.
	Secondary CPU. Identical to the first CPU, without the subram mapping (because for CPU1, this is our primary RAM).
*/

OUTPUT_ARCH(m68k)
OUTPUT_FORMAT(binary)
__DYNAMIC  =  0; 

MEMORY
{
	rom (rx)    : ORIGIN = 0x00000000, LENGTH = 384K /* Up to 6x 27C512 */
	ram (wx)    : ORIGIN = 0x00060000, LENGTH = 256  /* Just enough for the vectors */
}

ENTRY(_start);

SECTIONS 
{
	. = 0x00000000;
	.text :
	{
		LONG(0); 	/* No stack pointer */
		LONG(_start);   /* Reset initial program counter */

		/* Here come all the other vectors:
		   08 Access fault
		   0c Address error
		   10 Illegal instruction
		   14 Integer division by zero
		   18 CHK, CHK2 Instruction
		   1C FTRAPcc, TRAPcc, TRAPV instructions.
		   20 Privilige violation.
		   24 Trace.
		   28 Line 1010 Emulator
		   2C Line 1111 Emulator
		   30 Unassigned, reserved.
		   34 Coprocessor protocol violation.
		   38 Format error.
		   3c Uninitialized interrupt.
		*/

		/* Interrupt table. These point to jumps to the same addresses in RAM */
		/* Note that it's technically impossible to get any other irq than 2 or 4. */
		. = 0x60;
		LONG(_irq0_relay);
		LONG(_irq1_relay); /* Level 1 */
		LONG(_irq2_relay);
		LONG(_irq3_relay);
		LONG(_irq4_relay);
		LONG(_irq5_relay);
		LONG(_irq6_relay);
		LONG(_irq7_relay); /* Level 7 */

		. = 0x80; /* User Trap Vectors */
		LONG(_trap_0_relay);
		LONG(_trap_1_relay);
		LONG(_trap_2_relay);
		LONG(_trap_3_relay);
		LONG(_trap_4_relay);
		LONG(_trap_5_relay);
		LONG(_trap_6_relay);
		LONG(_trap_7_relay);
		LONG(_trap_8_relay);
		LONG(_trap_9_relay);
		LONG(_trap_a_relay);
		LONG(_trap_b_relay);
		LONG(_trap_c_relay);
		LONG(_trap_d_relay);
		LONG(_trap_e_relay);
		LONG(_trap_f_relay);

		. = 0x100;
		*(.text .text.*)

		. = ALIGN(2);
		*(.rodata .rodata.*)
	} > rom

	/* Provide RAM mirrors of the vector table here */
	.ramvect (NOLOAD) : 
	{
		*(.ramvect .ramvect.*)

                . = 0xefc;
                _magic_value = .;

	} > ram
}
