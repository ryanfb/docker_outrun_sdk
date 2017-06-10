/*******************************************************************************************
 *
 *  Out Run/X-Board-style road chip
 *
 *  Road control register:
 *      Bits               Usage
 *      -------- -----d--  (X-board only) Direct scanline mode (1) or indirect mode (0)
 *      -------- ------pp  Road enable/priorities:
 *                            0 = road 0 only visible
 *                            1 = both roads visible, road 0 has priority
 *                            2 = both roads visible, road 1 has priority
 *                            3 = road 1 only visible
 *
 *  Road RAM:
 *      Offset   Bits               Usage
 *      000-1FF  ----s--- --------  Road 0: Solid fill (1) or ROM fill
 *               -------- -ccccccc  Road 0: Solid color (if solid fill)
 *               -------i iiiiiiii  Road 0: Index for other tables (if in indirect mode)
 *               -------r rrrrrrr-  Road 0: Road ROM line select
 *      200-3FF  ----s--- --------  Road 1: Solid fill (1) or ROM fill
 *               -------- -ccccccc  Road 1: Solid color (if solid fill)
 *               -------i iiiiiiii  Road 1: Index for other tables (if in indirect mode)
 *               -------r rrrrrrr-  Road 1: Road ROM line select
 *      400-7FF  ----hhhh hhhhhhhh  Road 0: horizontal scroll
 *      800-BFF  ----hhhh hhhhhhhh  Road 1: horizontal scroll
 *      C00-FFF  ----bbbb --------  Background color index
 *               -------- s-------  Road 1: stripe color index
 *               -------- -a------  Road 1: pixel value 2 color index
 *               -------- --b-----  Road 1: pixel value 1 color index
 *               -------- ---c----  Road 1: pixel value 0 color index
 *               -------- ----s---  Road 0: stripe color index
 *               -------- -----a--  Road 0: pixel value 2 color index
 *               -------- ------b-  Road 0: pixel value 1 color index
 *               -------- -------c  Road 0: pixel value 0 color index
 *
 *  Logic:
 *      First, the scanline is used to index into the tables at 000-1FF/200-3FF
 *          - if solid fill, the background is filled with the specified color index
 *          - otherwise, the remaining tables are used
 *
 *      If indirect mode is selected, the index is taken from the low 9 bits of the
 *          table value from 000-1FF/200-3FF
 *      If direct scanline mode is selected, the index is set equal to the scanline
 *          for road 0, or the scanline + 256 for road 1
 *
 *      The horizontal scroll value is looked up using the index in the tables at
 *          400-7FF/800-BFF
 *
 *      The color information is looked up using the index in the table at C00-FFF. Note
 *          that the same table is used for both roads.
 *
 *
 *  Out Run road priorities are controlled by a PAL that maps as indicated below.
 *  This was used to generate the priority_map. It is assumed that X-board is the
 *  same, though this logic is locked inside a Sega custom.
 *
 *  RRC0 =  CENTA & (RDA == 3) & !RRC2
 *      | CENTB & (RDB == 3) & RRC2
 *      | (RDA == 1) & !RRC2
 *      | (RDB == 1) & RRC2
 *
 *  RRC1 =  CENTA & (RDA == 3) & !RRC2
 *      | CENTB & (RDB == 3) & RRC2
 *      | (RDA == 2) & !RRC2
 *      | (RDB == 2) & RRC2
 *
 *  RRC2 = !/HSYNC & IIQ
 *      | (CTRL == 3)
 *      | !CENTA & (RDA == 3) & !CENTB & (RDB == 3) & (CTRL == 2)
 *      | CENTB & (RDB == 3) & (CTRL == 2)
 *      | !CENTA & (RDA == 3) & !M2 & (CTRL == 2)
 *      | !CENTA & (RDA == 3) & !M3 & (CTRL == 2)
 *      | !M0 & (RDB == 0) & (CTRL == 2)
 *      | !M1 & (RDB == 0) & (CTRL == 2)
 *      | !CENTA & (RDA == 3) & CENTB & (RDB == 3) & (CTRL == 1)
 *      | !M0 & CENTB & (RDB == 3) & (CTRL == 1)
 *      | !M1 & CENTB & (RDB == 3) & (CTRL == 1)
 *      | !CENTA & M0 & (RDB == 0) & (CTRL == 1)
 *      | !CENTA & M1 & (RDB == 0) & (CTRL == 1)
 *      | !CENTA & (RDA == 3) & (RDB == 1) & (CTRL == 1)
 *      | !CENTA & (RDA == 3) & (RDB == 2) & (CTRL == 1)
 *
 *  RRC3 =  VA11 & VB11
 *      | VA11 & (CTRL == 0)
 *      | (CTRL == 3) & VB11
 *
 *  RRC4 =  !CENTA & (RDA == 3) & !CENTB & (RDB == 3)
 *      | VA11 & VB11
 *      | VA11 & (CTRL == 0)
 *      | (CTRL == 3) & VB11
 *      | !CENTB & (RDB == 3) & (CTRL == 3)
 *      | !CENTA & (RDA == 3) & (CTRL == 0)
 *
 *******************************************************************************************/

#ifndef __ROAD_H__
#define __ROAD_H__

// Same height as usual; we have 256 entries, but only 224 are visible.
// VBLANK occurs at line 224, but the road flip (when scheduled) happens on line 0.
#define ROAD_SCREEN_HEIGHT ((unsigned short)224)

#ifdef CPU0

// 16 entries; 8 for each road.
#define ROAD_PATTERN_PALETTE_START ((unsigned short)0x400)
// 32 possible background colors.
#define ROAD_BACKGROUND_PALETTE_START ((unsigned short)0x420)
// 128 solid line colors.
#define ROAD_SOLID_PALETTE_START ((unsigned short)0x780)

// Main CPU palette reset to a single color (pattern, background and solid).
void ROAD_ResetPalette (unsigned short bgColor);

#endif // CPU0

#ifdef CPU1

#include "subcpu.h"

#define ROAD_PATTERN_0 ((unsigned short*)(SUB_ROAD_BASE))
#define ROAD_PATTERN_1 ((unsigned short*)(SUB_ROAD_BASE + (0x200 >> 1)))
#define ROAD_SCROLL_0  ((unsigned short*)(SUB_ROAD_BASE + (0x400 >> 1)))
#define ROAD_SCROLL_1  ((unsigned short*)(SUB_ROAD_BASE + (0x800 >> 1)))
#define ROAD_COLORMAP  ((unsigned short*)(SUB_ROAD_BASE + (0xC00 >> 1)))

// Use this flag in the pattern lists to draw a solid scanline.
// Solid colors are the 128 colors starting at palette index 0x780.
#define ROAD_SOLID_COLOR(i) ((unsigned short)(0x800 + (i & 0x7f)))

// Center value for road scrolling. Tested in Mame, not on actual hardware.
#define ROAD_SCROLL_CENTER (unsigned short)0x5b4;

// Sets road priorities. From Mame's segaic16.c:
//   0 = road 0 only visible
//   1 = both roads visible, road 0 has priority
//   2 = both roads visible, road 1 has priority
//   3 = road 1 only visible
void ROAD_SetPriority (unsigned char priority);

// Schedules a buffer swap, which will occur on scanline 0. This will swap the RAM
// that is drawn by the road hardware with the RAM that is being written to at SUB_ROAD_BASE.
// Somewhat careful timing is needed, which Mame doesn't emulate properly.
// Since we only have an interrupt on the VBLANK at scanline 223, we should schedule the flip
// and finish writing the RAM contents before line 262/0, or delay everything until that line.
void ROAD_ScheduleSwapBuffers ();

// Safe swap for initialization code. Could take two frames but is safe to call at any time.
void ROAD_SwapBuffers ();

// Reset to a single background color (0x7f, palette entry 0x7ff).
void ROAD_Reset ();

#endif // CPU1

#endif // __ROAD_H__
