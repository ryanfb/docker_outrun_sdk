.text

.global orsound_init
.global orsound_update
.global orsound_write_command
.global orsound_enable
.global orsound_write_register

orsound_init:
  /* Reset our sound variables */
  clr.b sound_enabled
  clr.b sound_data_length
  clr.w sound_write_position
  clr.w sound_read_position
  clr.b sound_timer
  
  /* Clear registers */
  lea sound_registers, %A0
  mov.w #7, %D0
sound_clear_register_loop:
  clr.b (%A0, %D0)
  dbra %D0, sound_clear_register_loop
  rts

orsound_update:
  /* Store registers on the stack */  
  movem.l %D0-%D1/%A0, -(%A7)

  tst.b sound_enabled
  beq   sound_end
  
  /* What tick are we on? */
  movb sound_timer, %D0
  andi.w #7, %D0
  beq sound_next_command
  
  /* Continue writing register contents */
  lea sound_registers, %A0
  movb (%A0,%D0.w), %D1
  movb %D1, 0xffffff07.l
    
  /* Increase tick counter */
  addi.w #1, %D0
  moveb %D0, sound_timer
  bra sound_end
  
sound_next_command:
  /* Check whether we have commands left */
  tst.b sound_data_length
  beq sound_no_data
  
  /* Read command into D1 */
  lea sound_buffer, %A0
  move.w sound_read_position, %D0
  move.b (%A0,%D0.w), %D1

  /* Next buffer position */  
  addi.w #1, %D0
  andi.w #0x1f, %D0
  move.w %D0, sound_read_position
  subi.b #1, sound_data_length
  bra sound_write_command_irq

sound_no_data:
  moveb #0x80, %D1

sound_write_command_irq:
  movb %D1, 0xffffff07.l
  movb #1, sound_timer
  
sound_end:
  movem.l (%A7)+, %D0-%D1/%A0
  rts

/* Fixme: this should reset pointers and all */  
orsound_enable:
  move.l (0x4, %A7), %D0
  move.b %D0, sound_enabled
  rts
  
orsound_write_register:
  move.l (0x4, %A7), %D0
  move.l (0x8, %A7), %D1
  andi.l #7, %D0
  lea sound_registers, %A0
  move.b %D1, (%A0, %D0)
  rts
  
orsound_write_command:
  tst.b sound_enabled
  beq   sound_write_command_end

  move.w sound_write_position, %D0
  move.l (0x4, %A7), %D1
  lea sound_buffer, %A0
  move.b %D1, (%A0, %D0)
  addi.w #1, %D0
  andi.w #0x1f, %D0
  move.w %D0, sound_write_position
  addi.b #1, sound_data_length

sound_write_command_end:
  rts

.data

/* Sound variables */
sound_enabled: .byte 0					/* 0x6000c */
sound_timer: .byte 0					/* 0x6000d */
sound_write_position: .word 0			/* 0x6000e */
sound_read_position: .word 0			/* 0x60010 */
sound_data_length: .byte 0				/* 0x60012 */
sound_buffer: .skip 32

/* Sound registers; the first one is unused */
sound_registers:
.rept 8
.byte 0
.endr
