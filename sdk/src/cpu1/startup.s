# Out Run sub CPU init.
	
.extern main

/* From the linker script */
.extern __bss_start
.extern __bss_end
.extern __data_start
.extern __data_end
.extern __data_rom_start

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

  /* Copy data section contents from ROM; aligned to word boundaries. */
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

  /* To user mode, with interrupts enabled. */
  pea    _start_user
  move.w #0x000, -(%A7)
  rte

_start_user:
  /* Set up different stack pointer */
  lea    _stack_user, %A7
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
