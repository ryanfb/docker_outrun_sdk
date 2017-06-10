#ifndef __MAINCPU_H__
#define __MAINCPU_H__

/*
	These are the address ranges as seen by the main CPU.
	They are set up through our init code, since they're mapped by the 315-5195 memory mapper.
*/

#if defined(CPU1) || !defined(CPU0)
#error Address range conflict. Choose either main or sub CPU.
#endif

#define MAIN_ROM_BASE    ((const unsigned short*)0x000000)
#define MAIN_RAM_BASE    ((unsigned short*)0x060000)
#define TILE_RAM_BASE    ((unsigned short*)0x100000)
#define PALETTE_RAM_BASE ((unsigned short*)0x120000)
#define SPRITE_RAM_BASE  ((volatile unsigned short*)0x130000)
#define IO_REGS_BASE     ((volatile unsigned short*)0x140000)
#define SUB_ROM_BASE     ((const unsigned short*)0x200000)
#define SUB_RAM_BASE     ((volatile unsigned short*)0x260000)

#endif // __MAINCPU_H__
