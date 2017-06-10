/* 
	Sega Out Run hardware linker script.
	Secondary CPU.
*/

OUTPUT_ARCH(m68k)
OUTPUT_FORMAT(binary)
__DYNAMIC  =  0; 

MEMORY
{
	rom (rx)    : ORIGIN = 0x00000000, LENGTH = 384K /* Up to 6x 27C512 */
	ram (wx)    : ORIGIN = 0x00060000, LENGTH = 32K
}

/* Set up the stack */
SUPER_STACK_SIZE = 0x100;
USER_STACK_SIZE  = 0x200;

PROVIDE (_stack_super       = 0x00068000);
PROVIDE (_stack_user        = _stack_super - SUPER_STACK_SIZE);
PROVIDE (_stack_user_bottom = _stack_user - USER_STACK_SIZE); /* Can be checked by a heap implementation */

ENTRY(_start);

SECTIONS 
{
	. = 0x00000000;
	.text :
	{
		LONG(_stack_super); /* Initial stack pointer */
		LONG(_start);       /* Reset initial program counter */

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

		. = 0x60; /* Interrupts */
		LONG(__dummy_irq_handler);
		LONG(__dummy_irq_handler); /* Level 1 */
		LONG(__dummy_irq_handler);
		LONG(__dummy_irq_handler);
		LONG(__irq_4_handler);
		LONG(__dummy_irq_handler);
		LONG(__dummy_irq_handler);
		LONG(__dummy_irq_handler); /* Level 7 */

		. = 0x200;
		*(.text .text.*)

		. = ALIGN(0x4);
		__CTOR_LIST__ = .;
		___CTOR_LIST__ = .;
		LONG((__CTOR_END__ - __CTOR_LIST__) / 4 - 2)
		*(.ctors)
		LONG(0)
		__CTOR_END__ = .;
		__DTOR_LIST__ = .;
		___DTOR_LIST__ = .;
		LONG((__DTOR_END__ - __DTOR_LIST__) / 4 - 2)
		*(.dtors)
		LONG(0)
		__DTOR_END__ = .;

		*(.rodata .rodata.*)

		. = ALIGN(0x4);
		*(.gcc_except_table .gcc_except_table.*)

		. = ALIGN(0x4);
		*(.eh_frame) 

		. = ALIGN(0x2);
		__INIT_SECTION__ = . ;
		LONG (0x4e560000)       /* linkw %fp,#0 */ 
		*(.init)
		SHORT (0x4e5e)	        /* unlk %fp */ 
		SHORT (0x4E75)	        /* rts */

		__FINI_SECTION__ = . ;
		LONG (0x4e560000)       /* linkw %fp,#0 */
 		*(.fini)
		SHORT (0x4e5e)          /* unlk %fp */ 
		SHORT (0x4E75)          /* rts */

		_etext = .; 
		*(.lit)

		. = ALIGN(0x2);
		__data_rom_start = .;
	} > rom

	/* Initialized data; allocated in RAM, written to ROM (copied on startup) */
	.data :
	{
		. = ALIGN (0x2);
		__data_start = .;

		*(.data .data.*)

		. = ALIGN (0x2);
		__data_end = .;

	} > ram AT > rom

	/* Uninitialized (zero) data */
	.bss (NOLOAD) :
	{
		. = ALIGN (0x2);
		__bss_start = . ;

		*(.shbss)
		*(.bss .bss.*)
		*(COMMON)

		_end = ALIGN (0x2);
		__bss_end = . ;
		__end = _end;
		end = _end;
	} > ram
}
