# Out Run main CPU init.
	
.extern main

/* From the linker script */
.extern __bss_start
.extern __bss_end
.extern __RUN_FROM_RAM__

/* Bogus reference to force irq.o to be included. This is never called. */
.section "bogus"
  .extern __dummy_irq_handler
  lea __dummy_irq_handler, %A0

.text
  
/* Entry point */
.global _start
.weak _start
.set _start, __start

__start:
  /* This initializes the memory controller - 16 registers (size+flags?, base address) */
  lea    0xffffff20.l, %A0
  lea    memory_map_table, %A1
  move.l #0xf, %D0
memory_mapper_loop:
  move.w (%A1)+, (%A0)+
  dbra   %D0, memory_mapper_loop
  
  /* Clear BSS section; aligned to word boundaries. */
  move.l #__bss_end, %D0 
  sub.l  #__bss_start, %D0
  lsr.l  #1, %D0
  beq    bss_end
  lea    __bss_start, %A0
  sub.l  #1, %D0
bss_loop:
  clr.w  (%A0)+
  dbra   %D0, bss_loop
bss_end:

  /* Conditional: skip if running from RAM; the data segment is writable from the start and does not need to be copied */
  lea _start, %A0
  cmpa.l #0x60000, %A0	/* < 60000h = ROM. */
  bcc data_end

  tst.w 0x140060.l /* Watchdog clear */

  /* Copy data section contents from ROM; aligned to word boundaries. */
  /* Only needed with ROM linker script */

.extern __data_start
.extern __data_end
.extern __data_rom_start

  move.l #__data_end, %D0
  sub.l  #__data_start, %D0
  lsr.l  #1, %D0
  beq    data_end
  lea    __data_start, %A0
  lea    __data_rom_start, %A1
  sub.l  #1, %D0
data_loop:
  move.w (%A1)+, (%A0)+
  dbra   %D0, data_loop
data_end:

  tst.w  0x140060.l /* Watchdog clear */

  /* Set up a super mode stack */

.extern _stack_super

  lea _stack_super, %A7

  /* Do any super required stuff here */
  
  /* ... */
  
  /* To user mode, with interrupts enabled. */
  pea    _start_user
  move.w #0x000, -(%A7)
  rte

_start_user:
  /* Set up different stack pointer */
  lea    _stack_user, %A7
  
  /* Run init code (*.init) */
  .extern __INIT_SECTION__
  jsr    __INIT_SECTION__  
  
  jmp    main

/* Trap #0 - Set IRQ level */
.global __trap0_set_irq_level
__trap0_set_irq_level:
    /* Set IRQ mask. D0 = Level. */
    andi.w #7, %D0
    asl.w #8, %D0                /* D0 <<= 8 -> D0 = 0x700 */
    move.w (%A7)+, %D1           /* Take old status value. */
    andi.w #0xf8ff, %D1          /* Mask out old IRQ bits  */
    or.w %D0, %D1                /* Or in new IRQ bits. */
    move.w %D1, -(%A7)           /* Put the status back where it was. */
    rte

.global __halt_no_interrupts
__halt_no_interrupts:
    stop #0x2700
	
/* Memory controller (315-5195) setup table. */
/* This is the same table used in Out Run rev. B, so all of our addresses are identical. */

memory_map_table:

.word 0x0002 /* size = 0x7ffff */
.word 0x0000 /* addr = 0x00000000  ROM / Main RAM */

.word 0x000d /* size = 0x1ffff     flags = 0xC? */
.word 0x0010 /* addr = 0x00100000  tile RAM + text RAM */

.word 0x0000 /* size = 0xffff */
.word 0x0012 /* addr = 0x00120000  palette RAM */

.word 0x000c /* size = 0xffff      flags = 0xC? */

.word 0x0013 /* addr = 0x00130000  sprite/object RAM */

.word 0x0008 /* size = 0xffff      flags = 0x8? */
.word 0x0014 /* addr = 0x00140000  I/O Space */

.word 0x000f /* size = 0x1fffff    flags = 0xC? */
.word 0x0020 /* addr = 0x00200000  CPU 1 ROM/RAM */

.word 0x0000, 0x0000, 0x0000, 0x0000 /* Unused */
