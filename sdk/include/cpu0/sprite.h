// The following description is from MAME's /video/sega16sp.cpp
// license:BSD-3-Clause
// copyright-holders:Aaron Giles 

//
//  Out Run/X-Board-style sprites
//
//      Offs  Bits               Usage
//       +0   e------- --------  Signify end of sprite list
//       +0   -h-h---- --------  Hide this sprite if either bit is set
//       +0   ----bbb- --------  Sprite bank
//       +0   -------t tttttttt  Top scanline of sprite + 256
//       +2   oooooooo oooooooo  Offset within selected sprite bank
//       +4   ppppppp- --------  Signed 7-bit pitch value between scanlines
//       +4   -------x xxxxxxxx  X position of sprite (position $BE is screen position 0)
//       +6   -s------ --------  Enable shadows
//       +6   --pp---- --------  Sprite priority, relative to tilemaps
//       +6   ------vv vvvvvvvv  Vertical zoom factor (0x200 = full size, 0x100 = half size, 0x300 = 2x size)
//       +8   y------- --------  Render from top-to-bottom (1) or bottom-to-top (0) on screen
//       +8   -f------ --------  Horizontal flip: read the data backwards if set
//       +8   --x----- --------  Render from left-to-right (1) or right-to-left (0) on screen
//       +8   ------hh hhhhhhhh  Horizontal zoom factor (0x200 = full size, 0x100 = half size, 0x300 = 2x size)
//       +E   dddddddd dddddddd  Scratch space for current address
//
//  Out Run only:
//       +A   hhhhhhhh --------  Height in scanlines - 1
//       +A   -------- -ccccccc  Sprite color palette
//
//  X-Board only:
//       +A   ----hhhh hhhhhhhh  Height in scanlines - 1
//       +C   -------- cccccccc  Sprite color palette
//
//  Final bitmap format:
//
//            -s------ --------  Shadow control
//            --pp---- --------  Sprite priority
//            ----cccc cccc----  Sprite color palette
//            -------- ----llll  4-bit pixel data
// 

#ifndef __SPRITE_H__
#define __SPRITE_H__

#include "maincpu.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" 
{
#endif // __cplusplus

// C structure for a single sprite list entry as described above.
typedef volatile struct
{
	// 0x0
	uint16_t EndOfList : 1;
	uint16_t Hidden : 1;
	uint16_t : 1;
	uint16_t Hidden2 : 1;
	uint16_t Bank : 3;
	uint16_t Top : 9;
	
	// 0x2
	uint16_t BankOffset; // Offset within the bank, per 4 bytes.
	
	// 0x4
	int16_t  Pitch : 7; // Pitch in 8-pixel segments (4 bytes).
	uint16_t Left : 9; // 0xbe = screen pos 0.
	
	// 0x6
	uint16_t : 1;
	uint16_t EnableShadows : 1;
	uint16_t Priority : 2;
	uint16_t : 2;
	uint16_t VerticalZoomFactor : 10; // 0x200 = full size.
	
	// 0x8
	uint16_t DrawTopToBottom : 1;
	uint16_t FlipHorizontal : 1;
	uint16_t DrawLeftToRight : 1; // So.. why do we have two of these? Relative to sprite/screen?
	uint16_t : 3;
	uint16_t HorizontalZoomFactor : 10; // 0x200 = full size.

	// 0xA	
	uint16_t Height : 8;   // -1; 'last line index'.
	uint16_t : 1;
	uint16_t PaletteIdx : 7;
	
	uint16_t : 16;         // 0xC
	uint16_t ScratchSpace; // 0xE
} SpriteData;

// Sprite RAM is located in 2x TMM 2018 (IC51 and 52), which gives us a total of 4KB.
// A10 is connected to A12 on the sprite generator; we're skipping a bit there (the generator may support 8KB).
// The CPU side only connects bits 1-10, so it doesn't know which bank it's writing into.
// My guess is that A12 is inverted by the generator when it detects a CPU write (XCPU). Todo: verify by measuring.

#define SPRITE_RAM_SIZE (2*2048)
#define SPRITE_RAM_BUFFER_SIZE 2048
#define SPRITE_LIMIT 128

// Top left of the visible screen.
#define SPRITE_ORIGIN_LEFT 0xbe
#define SPRITE_ORIGIN_TOP  0x100

// Just go on your merry way and use it like SpriteList[0].EndOfList = 1;
#define SpriteList ((SpriteData*)SPRITE_RAM_BASE)

// The SCHG/XCHG signal (write to I/O address 140070) will also toggle between these two halves.
// It's possible that it'll latch-and-wait until a GXINT signal, just like the road generator does.
// Todo: measure timing.
// The initial sprite buffer flip is implemented as 'not.w $140070.l' in the original ROM set.
static inline void SPRITE_SwapBuffers (void) { __asm__("not.w 0x140070.l"); }

// CONT and SG1 (-> 'XMRS') lines are connected from PPI port C (output) to the sprite generator.
// They're not currently being emulated in MAME.
// Add PPI wrapper functions here, if necessary.

#ifdef __cplusplus
} // extern "C"
#endif // __cplusplus

#endif // __SPRITE_H__
