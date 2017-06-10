/* 
   Out Run boot loader for use with PC adapter (main cpu part)
   (c) 2012-2015 Alex Bartholomeus aka deadline/excessive force

   Todo:
   - Exception vectors.
   - Progress bar and fancy graphics?
*/   

/* Used memory addresses */
.equ TILERAM_BASE,    0x100000
.equ TEXTRAM_BASE,    0x110000
.equ TILE_REGISTERS,  (TEXTRAM_BASE+0xe80)
.equ PALETTE_BASE,    0x120000
.equ SPRITE_BASE,     0x130000
.equ IO_BASE,         0x140000
.equ SUBRAM_BASE,     0x260000

/* I/O registers */
.equ IO_PPI_PORT_C,  (IO_BASE+0x5)
.equ IO_PPI_CONTROL, (IO_BASE+0x7)
.equ IO_DIGITAL_OUT, (IO_BASE+0x21)
.equ IO_WATCHDOG,    (IO_BASE+0x60)

/* LPT status flags */
.equ LPT_STATUS_BUSY,  0x08 
.equ LPT_STATUS_nACK,  0x10
.equ IO_EXTERNAL_MUTE, 0x80

/* Generic watchdog clear macro */
.macro WATCHDOG_CLEAR
  tst.w IO_WATCHDOG
.endm

/* Call() macro, used to emulate call like behaviour without stack. Returns to the address in %A7. */
.macro CALL function
  movea.l #1f, %A7
  jmp \function
1: /* Return address */
.endm

.macro CALL_RETURN function returnaddr
  movea.l #\returnaddr, %A7
  jmp \function
.endm

/* Return macro; jumps back to the address stored in %A7 */
.macro RETURN
  jmp (%A7)
.endm

/*
   Entry point.
*/
.text
.global _start
_start:

  /* This initializes the memory controller - 16 registers (size+flags?, base address) */
  lea    0xffffff20.l, %A0
  lea    memory_map_table, %A1
  move.l #0xf, %D0
memory_mapper_loop:
  move.w (%A1)+, (%A0)+
  dbra   %D0, memory_mapper_loop

  /* Reset second CPU, and zero out the initial program counter */
  reset
  nop
  nop
  clr.l SUBRAM_BASE+4
  clr.l SUBRAM_BASE
  nop
  nop
  reset

  WATCHDOG_CLEAR

  /* Set up a dummy IRQ table in RAM */
  move.l #_dummyirq, %A0
  move.l %A0, _irq0_ram
  move.l %A0, _irq1_ram
  move.l %A0, _irq2_ram
  move.l %A0, _irq3_ram
  move.l %A0, _irq4_ram
  move.l %A0, _irq5_ram
  move.l %A0, _irq6_ram
  move.l %A0, _irq7_ram

  /* Clone our initial PC in RAM, just in case. */
  move.l #_start, %A0
  move.l %A0, _pc_ram

  WATCHDOG_CLEAR
  
  /* Init PPI, No audio and no screen. */
  move.b #0x90, IO_PPI_CONTROL /* Control register; A=in; B/C=out */
  clr.b  IO_PPI_PORT_C         /* Clear port C, resetting audio and blanking the screen */

  /* Reset digital output, BUSY(0x8) and ACK(0x10) should be high */
  move.b #0x98, IO_DIGITAL_OUT /* Default value: 0x81; Internal and external mute */

  /* Clear sprites, put an empty entry at the start of the list and flip */
  move.l #0xc0000000, SPRITE_BASE
  clr.l (SPRITE_BASE+0x4)
  clr.l (SPRITE_BASE+0x8)
  clr.l (SPRITE_BASE+0xc)
  move.b #0xff, 0x140070

  /* Set road palette to background color */
  cmp.l #0xEFC2014, _magic_value
  bne _sub_not_present  
  move.w #0x0842, %D0 /* Pick a nice color here */
  jmp _bgcolor_set;
_sub_not_present:
  move.w #0x0666, %D0 /* Pick a dreary color here */
_bgcolor_set:
  move.l #(PALETTE_BASE+(0x400*2)), %A0
  move.l #(PALETTE_BASE+(0x420*2)), %A1
  move.l #(PALETTE_BASE+(0x780*2)), %A2
  move.w #0xf, %D1
_roadpalette_loop:
  move.w %D0, (%A0)+ /* 16 entries */
  move.w %D0, (%A1)+ /* 32 entries */
  move.w %D0, (%A1)+
  move.w %D0, (%A2)+ /* 128 entries */
  move.w %D0, (%A2)+
  move.w %D0, (%A2)+
  move.w %D0, (%A2)+
  move.w %D0, (%A2)+
  move.w %D0, (%A2)+
  move.w %D0, (%A2)+
  move.w %D0, (%A2)+
  dbra   %D1, _roadpalette_loop

  WATCHDOG_CLEAR

  /* Clear text layer */
  move.l #0x00200020, %D0
  move.w #0x37f, %D1
  move.l #TEXTRAM_BASE, %A0
_textclear_loop:
  move.l %D0, (%A0)+
  dbra   %D1, _textclear_loop

  WATCHDOG_CLEAR

  /* Set and clear one tile map (#0) */
  clr.w 0x110E80
  clr.w 0x110E82
  move.w #0x3ff, %D0
  move.l #0x00200020, %D1
  move.l #TILERAM_BASE, %A0
_tileclear_loop:
  move.l %D1, (%A0)+
  dbra %D0, _tileclear_loop

  WATCHDOG_CLEAR
  
  /* Reset all tile registers; this also sets the tile maps to use map #0 (which we just cleared) */
  move.w #0x5e, %D0
  move.l #TILE_REGISTERS, %A0
 _tilereg_loop:
  clr.l (%A0)+
  dbra %D0, _tilereg_loop
  
  WATCHDOG_CLEAR

  /* Init text palette */
  movea #_textpalette, %A0
  move.l #PALETTE_BASE, %A1
  move.w #0x17, %D0
_textpalette_loop:
  move.w (%A0)+, (%A1)+
  dbra %D0, _textpalette_loop

  /* Enable the screen */
  move.b #0x20, IO_PPI_PORT_C
 
  /* Print title message */
  movea #_titlemessage, %A0
  move.l #(TEXTRAM_BASE+((26+2*64)*2)), %A1
  clr.w %D0
_msg_loop:
  move.b (%A0)+, %D0
  beq _msg_end
  move.w %D0, (%A1)+
  bra _msg_loop
_msg_end:


/*
   Main command loop.
*/
_mainloop:

  /* Print that we are currently idle */
  movea #_status_idle, %A0
  move.w #0x400, %D0 /* Yellow */
  CALL _print_status

  WATCHDOG_CLEAR

  /* Read a single command byte into %D0 */
  CALL _readchar

  /* NOP command */
  cmp.b #0x0, %D0
  beq _mainloop

  /* Reboot to RAM */
  cmp.b #0x1, %D0
  beq _rebootram

  /* Reboot to ROM */
  cmp.b #0x2, %D0
  beq _rebootrom

  /* Upload data to main RAM */
  cmp.b #0x3, %D0
  beq _uploadmainram

  /* Upload data to sub RAM */
  cmp.b #0x4, %D0
  beq _uploadsubram

  /* Invalid command */
  movea.l #_status_invalid, %A0
  move.w #0x200, %D0 /* Red */
  CALL _print_status
  move.l #500, %D0
  CALL_RETURN _sleep, _mainloop


/* 
   Reads a single character. Return value in low byte of %D0. Leaves BUSY high afterwards.
   Return address in %A7 
*/
_readchar:
  /* Set BUSY to low; output register now 0x90 */
  move.b #0x90, IO_DIGITAL_OUT

  /* Wait for STROBE to become low. */
_readchar_wait_strobe_low:
  WATCHDOG_CLEAR                   /* 20 cycles */
  btst.b #0, 0x140011              /* 10 cycles */
  bne _readchar_wait_strobe_low    /* 16 cycles */

  /* Set BUSY to high. */
  move.b #0x98, IO_DIGITAL_OUT

  /* Read data, from digital input 2 (register 1) */
  move.b 0x140013, %D0

  /* Now pulse ACK to low to indicate we have got our data, 0.5-10 microseconds. Not used. */
  move.b #0x88, IO_DIGITAL_OUT
  nop
  nop
  nop
  nop /* 1.6 microseconds. */

  /* Wait for STROBE to get high again first, since this can range 0.75-500 microseconds. */
_readchar_wait_strobe_high_end:
  WATCHDOG_CLEAR
  btst.b #0, 0x140011
  beq _readchar_wait_strobe_high_end

  /* Set nACK to high. */
  move.b #0x98, IO_DIGITAL_OUT

  /* Return to caller */
  RETURN


/*
   Reads a single word, high byte first. Result in %D0
   Modifies %A6.
*/
_readword:
  move.l %A7, %A6
  clr.w  %D0
  CALL   _readchar
  asl.w  #8, %D0
  CALL   _readchar
  move.l %A6, %A7
  RETURN


/* 
   Sleep function; D0 = amount of milliseconds (approximately); A7 = return address.
   Modifies D0 and D1
*/
_sleep:
  move.w #833, %D1 /* dbra is ~12 cycles, so this should be one millisecond */
_sleep_one_ms:
  dbra %D1, _sleep_one_ms
  WATCHDOG_CLEAR
  dbra %D0, _sleep
  RETURN


/*
   Reboots to the image in RAM.
*/
_rebootram:
  movea.l #_status_rebootram, %A0
  move.w #0x400, %D0 /* Yellow */
  CALL _print_status
  move.l #500, %D0 /* 500ms */
  CALL _sleep

  bclr #5, IO_PPI_PORT_C /* Blank screen */
  reset
  movea.l _sp_ram, %A7
  movea.l (_pc_ram), %A0
  jmp     (%A0)


/*
  Reboots to the image in ROM.
*/
_rebootrom:
  movea.l #_status_rebootrom, %A0
  move.w  #0x400, %D0 /* Yellow */
  CALL    _print_status
  move.l  #500, %D0 /* 500ms */
  CALL    _sleep

  bclr    #5, IO_PPI_PORT_C /* Blank screen */

  /* Dummy out the initial program counter of the secondary CPU */
  clr.l SUBRAM_BASE+4
  clr.l SUBRAM_BASE
  nop
  nop
  reset
  jmp     _start;

/*
   Uploads data to RAM sent through the I/O interface. Starts with the 16-bit size in high-low order.
*/
_uploadsubram:
  movea.l #_status_uploadsub, %A0
  move.w  #0x400, %D0 /* Yellow */
  CALL    _print_status

  movea.l #0x260000, %A0
  bra     _upload_shared

_uploadmainram:
  movea.l #_status_uploadram, %A0
  move.w  #0x400, %D0 /* Yellow */
  CALL    _print_status

  movea.l #0x60000, %A0

_upload_shared: /* Also used by _uploadsubram

  /* Read the number of bytes we mean to upload, in high-low order (word) */
  CALL   _readchar
  clr.l  %D1
  move.b %D0, %D1
  asl    #8, %D1
  CALL   _readchar
  move.b %D0, %D1

  /* If zero, bail. */
  cmp.w  #0, %D1
  beq    _upload_done

  /* Remove topmost bit. We could also clamp here. In any case this is unexpected. */
  subi    #1, %D1
  andi.w  #0x7fff, %D1

  /* This is the main upload loop, which will continue until we have all bytes */
_upload_loop:
  CALL    _readchar
  move.b  %D0, (%A0)+
  dbra    %D1, _upload_loop  

_upload_done:
  move.l  #500, %D0
  CALL_RETURN _sleep, _mainloop

/*
   Prints a status message.
   High byte of D0 word: color.
   A7 = Return address, A0 = Status message
   Modifies D0 and A1
*/
_print_status:
  move.l #(TEXTRAM_BASE+((26+(4*64))*2)), %A1
_print_status_loop:
  move.b (%A0)+, %D0
  beq    _print_status_end
  move.w %D0, (%A1)+
  bra    _print_status_loop
_print_status_end:
  RETURN


# Initialization table for the memory controller.
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

# IRQ relay entries. This is what the ROM irq table points to.
# We can't trash any registers here, so we put the status register and the relay address on the stack.

/* Helper to relay a vector to another vector in ram */
.macro relayvector prefix
.global \prefix\()_relay
\prefix\()_relay:
  move.l \prefix\()_ram, -(%A7)
  move.w %SR, -(%A7)
  rte
.endm

relayvector _irq0
relayvector _irq1
relayvector _irq2
relayvector _irq3
relayvector _irq4
relayvector _irq5
relayvector _irq6
relayvector _irq7

relayvector _trap_0
relayvector _trap_1
relayvector _trap_2
relayvector _trap_3
relayvector _trap_4
relayvector _trap_5
relayvector _trap_6
relayvector _trap_7
relayvector _trap_8
relayvector _trap_9
relayvector _trap_a
relayvector _trap_b
relayvector _trap_c
relayvector _trap_d
relayvector _trap_e
relayvector _trap_f
  
# Dummy IRQ routine.
_dummyirq:
  rte

# These are mirrors of the PC/SP and IRQ table in ram.
.section .ramvect
_sp_ram: .skip 4
_pc_ram: .skip 4

.org 0x60
_irq0_ram: .skip 4
_irq1_ram: .skip 4
_irq2_ram: .skip 4
_irq3_ram: .skip 4
_irq4_ram: .skip 4
_irq5_ram: .skip 4
_irq6_ram: .skip 4
_irq7_ram: .skip 4

.org 0x80
_trap_0_ram: .skip 4
_trap_1_ram: .skip 4
_trap_2_ram: .skip 4
_trap_3_ram: .skip 4
_trap_4_ram: .skip 4
_trap_5_ram: .skip 4
_trap_6_ram: .skip 4
_trap_7_ram: .skip 4
_trap_8_ram: .skip 4
_trap_9_ram: .skip 4
_trap_a_ram: .skip 4
_trap_b_ram: .skip 4
_trap_c_ram: .skip 4
_trap_d_ram: .skip 4
_trap_e_ram: .skip 4
_trap_f_ram: .skip 4

# Miscellaneous data, written to ROM.
.section .rodata

_titlemessage:
.asciz "  OUTRUN BOOTLOADER V0.9B EFC 2015 "

# Status messages
_status_idle:
.asciz "        WAITING FOR COMMAND   "
_status_rebootram:
.asciz "         REBOOTING TO RAM...  "
_status_rebootrom:
.asciz "         REBOOTING TO ROM...  "
_status_uploadram:
.asciz "      UPLOADING TO MAIN RAM..."
_status_uploadsub:
.asciz "       UPLOADING TO SUB RAM..."
_status_invalid:
.asciz "          INVALID COMMAND     "

.align 2
_textpalette:
.word 0x0000, 0x0999, 0x7DDD, 0x7FFF, 0x7FFF, 0x7FFF, 0x7FFF, 0x0111 /* White */
.word 0x0000, 0x6449, 0x766D, 0x199F, 0x1BBF, 0x7DDF, 0x7FFF, 0x0000 /* Red */
.word 0x0000, 0x4499, 0x76DD, 0x39FF, 0x3BFF, 0x7DFF, 0x7FFF, 0x0000 /* Yellow */

_hexlookup:
.ascii "0123456789ABCDEF"
