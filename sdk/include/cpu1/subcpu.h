#ifndef __SUBCPU_H__
#define __SUBCPU_H__

/*
	These are the address ranges as seen by the secondary CPU.
*/

#if defined(CPU0) || !defined(CPU1)
#error Address range conflict. Choose either main or sub CPU.
#endif

#define SUB_ROM_BASE     ((unsigned short*)0x000000)
#define SUB_RAM_BASE     ((unsigned short*)0x060000)
#define SUB_ROAD_BASE	 ((unsigned short*)0x080000)
#define SUB_ROAD_CONTROL ((volatile unsigned short*)0x090000)

// Waits for a vblank/GXINT interrupt (irq4, scanline 223).
// Blocks long enough to ensure we're on line 224 (thus only triggering once per frame).
void wait_vblank ();

#endif // __SUBCPU_H__
