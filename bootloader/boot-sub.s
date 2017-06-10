/* 
   Out Run boot loader for use with PC adapter (sub cpu part)
   (c) 2015 Alex Bartholomeus aka deadline/excessive force

   Todo:
   - Exception vectors.
*/   

/*
   Entry point.
*/
.text
.global _start
_start:

  /* Insert some nop operations to make sure that our RAM start vector isn't being accessed while the main CPU sets it. */
  nop
  nop
  nop
  nop
  nop
  nop
  nop
  nop
  nop
  nop
  nop
  nop
  nop
  nop
  nop
  nop

  /* Our RAM program counter is now either zero or set to the actual start address */  
  cmp.l #0, _pc_ram
  bne _start_ram

  /* Set a magic value so the main CPU knows the correct roms are present */  
  move.l #0xefc2014, _magic_value

  /* Clear the road background, just in case we didn't build CPU1 code. */
  /* Set road 0 to all solid color #0 */
  move.l #0x08000800, %D0
  move.w #0x7f, %D1
  move.l #0x80000, %A0
_roadclear_loop:
  move.l %D0, (%A0)+
  dbra   %D1, _roadclear_loop

  /* Swap buffers (and pray for the right timing - we can't enable interrupts) */
  clr.b 0x90001 /* Set prio 0 = draw road 0 only */
  tst.b 0x90001 /* Swap */
  
  /* Wait until we get a reset again */
  stop #0x2700

_start_ram:
  movea.l _sp_ram, %A7
  movea.l (_pc_ram), %A0
  jmp     (%A0)
  
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
