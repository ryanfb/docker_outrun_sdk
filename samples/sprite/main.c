#include "hwinit.h"
#include "text.h"
#include <sprite.h>
#include <irq.h>
#include <string.h> // memset
#include "palette.h"
#include "input.h"
#include "menu.h"
#include <io.h>

/*

 ----------------------------------------------------------------------------
 Sprites
 ----------------------------------------------------------------------------

 The 315-5211A has acess to 8K of RAM divided into two 4K banks, and 512K 
 of RAM divided into two 512x256 16-bit framebuffers. The 315-5211A controls
 selection of which 4K bank is available.

 Generally speaking the 315-5211A parses one list while giving the CPU access
 to the other, and renders to one framebuffer while displaying the other.
 Writing to $110000 triggers the rendering sequence and further writes are
 ignored until rendering stops.

 The only indicator the 315-5211A gets about the display state is the
 VBlank interrupt, so it most likely stops rendering at that point.

 Each 4K bank of sprite RAM contains 256 16-byte entries describing a single
 sprite. The sprites are rendered to the framebuffer in the same order they
 appear in the list, so sprite #255 overwrites sprite #0.

 With enough sprites displayed it is possible to have the render process
 aborted due to a lack of time, in which case the last sprite being drawn
 may only be partially drawn to the framebuffer.

 Sprites are clipped when drawn to the framebuffer; any portion of a sprite
 that exceeds the 512x256 framebuffer space is not drawn.

 Sprite ROM data is arranged as 256K banks (64Kx32-bits) Each 32-bit word
 read from sprite ROM holds eight 4-bit pixels. The pitch is added to the
 16-bit offset after each line rendered, much like System 16B.

 For pixel data, $0 is transparent, $A is a shadow/hilight pen when a sprite's
 shadow/hilight enable bit is set, and $F is an end marker (also transparent).

 The format of each entry in sprite RAM is:

 Offset $00

     D15 : End of sprite list (1= stop, 0= continue)
     D14 : Hide sprite (1= hide, 0= visible)
     D13 : ?
     D12 : Hide sprite (1= hide, 0= visible)
 D11-D10 : Sprite ROM bank select (fed to a PAL)
   D7-D0 : Starting line in framebuffer ($0000 = line 0)

 Offset $02

 D15-D0  : Sprite ROM bank offset

 Offset $04

 D15-D9  : Sprite pitch
 D8-D0   : Starting pixel in framebuffer ($00B8 = pixel 0)

 OFfset $06

 D15     : ?
 D14     : Shadow/hilight enable (1= Pen $A is shadow/hilight, 0= Pen $A is normal)
 D13-D12 : Sprite priority (relating to tilemaps only)
 D11-D0  : Sprite vertical zoom:
           $0000 = 8x tall
           $0080 = 4x tall
           $0100 = 2x tall
           $0200 = Normal height
           $03FF = 1/2 tall
     $0400-$05FF = 1/2 tall (all settings are the same as $400)
     $0600-$0700 = Normal height (doubled)
           $0780 = 2x tall (doubled)
           $07C0 = 4x tall (doubled)

 The zoom value is only valid for $0000-$03FF. Values of $0400-$05FF are
 identical, and from $0600-$7FF the sprite gradually becomes taller again
 but the wrong lines are doubled. I think this is unintentional and these
 values are not supposed to be used.

 Shadow/hilight mode is controlled by the tilemap chip, so I'm not sure
 how the road layer is affected.

 Offset $08

 D15     : Render vertical direction (1= start line going upwards, 0= start line going downwards)
 D14     : Sprite flip (1= flip, 0= normal) Relates to order in which end codes are parsed.
 D13     : Render horizontal direction (1= start pixel going left, 0= start pixel going right)
 D12     : Unknown. May disable or change end code pixel value.
 D11-D0  : Sprite horizontal zoom:
           $0000 = 8x wide
           $0080 = 4x wide
           $0100 = 2x wide
           $0200 = Normal width
           $0300 = 1/2 wide
           $0400 = 1/4 wide
     $0400-$0FFF = Invalid

 Bits 15 and 13 change the direction data is rendered to the framebuffer;
 e.g. setting bit 13 renders pixels in a line from the starting pixel going
 backwards, setting bit 15 renders pixels from the starting line going upwards.

 This does not change how the end codes are parsed; bit 14 is a true
 horizontal flip bit in that sense. (data still rendered in the order specified
 by bits 15, 13)

 Much like the Y-zooming, X-zoom values of $0400-$0FFF start off at 1/4th
 wide and gradually expand to reach 8x wide at the end ($0FFF). However these
 values are invalid and while the sprite shrinks down to something like 1/8th
 or 1/16th, there are many rendering errors (end codes skipped, all sorts of
 garbage) probably due to the wrong data being returned.

 Offset $0A

 D15-D12 : ?
 D11-D0  : Sprite height in framebuffer.

 The framebuffer is only 256 lines tall, so values larger than that are
 clipped.

 Offset $0C

 D15-D8  : ?
 D7-D0   : Sprite palette.

 Offset $0F

 Does nothing.
 
 */

// Car palette (one of them, at least - there's animation for the tires and brake lights)
// Palettes for sprites start at entry 0x800.
const uint16_t CarPalette[] = 
{ 
	// Ferrari
	0x0000, 0x0008, 0x033a, 0x055c, 0x066d, 0x088d, 0x0aad, 0x0ddd, 0x09ab, 0x089a, 0x007d, 0x0008, 0x0000, 0x0888, 0x0666, 0x000,
	
	// Ferrari flipping over.
	0x000, 0x000, 0xffc, 0x777, 0x888, 0x999, 0xaaa, 0xbbb, 0xccc, 0xddd, 0xfff, 0xaaf, 0x89f, 0x66e, 0x55c, 0x000,
	
	// Guy
	0x000, 0x047, 0x069, 0xbde, 0x9be, 0x87b, 0x658, 0xd78, 0xb60, 0x700, 0x000, 0xeee, 0xaaa, 0x777, 0x000, 0x000,
	
	// Girl
	0x000, 0x0be, 0x0ef, 0xbdf, 0x9ad, 0x88b, 0xbac, 0xa0c, 0x90a, 0x708, 0x000, 0xd98, 0xc68, 0x078, 0xda3, 0x000,
	
	// Trophy girl?
	0x000, 0x069, 0x0bd, 0xbdf, 0x9ad, 0x88b, 0x0ef, 0x9b7, 0x796, 0x574, 0x000, 0xfff, 0xcbb, 0x988, 0xfff, 0x000,
	
	// Car - Gold
	0x000, 0x056, 0x078, 0x19a, 0x0bc, 0x7cd, 0x9cd, 0xddd, 0xbbb, 0x999, 0x08c, 0x009, 0x000, 0xaaa, 0x888, 0x000,
	
	// Car - Blue
	0x000, 0x666, 0x844, 0xa44, 0xc66, 0x4e88, 0x4faa, 0xfff, 0xffd, 0xcc9, 0x07d, 0x00a, 0x000, 0xaaa, 0x777, 0x000,
	
	// Car - Gray
	0x000, 0x444, 0x555, 0x555, 0x666, 0x777, 0x999, 0xccc,  0xffd, 0xcc9, 0x07d, 0x008, 0x000, 0xaaa, 0x777, 0x000,
	
	// Foliage? No. Outrun Logo, possibly.
//	0x000, 0xfbe, 0xfde, 0xeec, 0xbf6, 0xbe0, 0xfce, 0xccc,  0xbab, 0x7b0, 0x890, 0x99b, 0x9ab, 0xabc, 0x777, 0x000,
	// Palm tree.
	0x000, 0xabc, 0x9ac, 0x89a, 0x899, 0x478, 0x579, 0x456, 0x496, 0x0a0, 0x897, 0x560, 0x090, 0x0c0, 0xddd, 0x000,
	
	// Big tree, possibly grasslands. Gradient inverted so it doesn't look good on all foliage.
	0x000, 0x9ff, 0x8fd, 0x7eb, 0x5c8, 0x4b5, 0x292, 0x170, 0x050, 0x030, 0x000, 0x56a, 0x348, 0x126, 0x014, 0x000,
	
	// Tunnels/ruins.
	0x000, 0x000, 0x332, 0x244, 0x355, 0x466, 0x677, 0x799, 0xaaa, 0x07a, 0x000, 0x055, 0x055, 0x07a, 0x0aa, 0x000, // Strange colors in last 4.
	0x000, 0x000, 0x443, 0x355, 0x466, 0x577, 0x788, 0x899, 0xbbb, 0x044, 0x000, 0x044, 0x087, 0x044, 0x044, 0x000,
	
	// Radio.
	0x000, 0xfff, 0x0f0, 0x00f, 0x000, 0x888, 0x999, 0xaaa, 0xbbb, 0xccc, 0xddd, 0x0f0, 0x0f0, 0x00f, 0x0f0, 0x000,
	0x000, 0xaef, 0x9df, 0x8cf, 0x7be, 0x0ad, 0x09c, 0x08c, 0xfff, 0x000, 0x000, 0x000, 0x000, 0x000, 0xdff, 0x000, // Arm
	0x000, 0xfff, 0x000, 0x888, 0x999, 0xaaa, 0xbbb, 0xccc, 0xddd, 0xeee, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, // Knob
	
	
};

enum
{
	Palette_Ferrari = 0,
	Palette_Ferrari2,
	Palette_Guy,
	Palette_Girl,
	Palette_TrophyGirl,
	Palette_Car_Gold,
	Palette_Car_Blue,
	Palette_Car_Gray,
	Palette_Palmtree,
	Palette_Foliage1,
	Palette_Stone1,
	Palette_Stone2,
	Palette_Radio,
	Palette_Radio_Arm,
	Palette_Radio_Knob,
};

typedef struct 
{
	uint8_t  BankIdx;
	uint16_t BankOffset; 	// In DWORDs (which is how the sprite generator addresses it)
	int8_t   Pitch;			// Pitch (8 pixels, or 32 bits)
	uint8_t  Height; 		// Total height (this is one more than the Height member set in the sprite registers).
	uint8_t  PaletteIdx; 	// Default palette.
} SpriteInfo;

#define Palette_Porsche Palette_Car_Blue
#define Palette_BikiniGirl Palette_Guy

// List generated by scanning for end markers in the sprite mask roms.
// Further split up by hand when sprites of the same pitch/width follow each other.
const SpriteInfo GameSprites[] = 
{
#if 1 
	//---------------------------------------------------------------------------------------------------------------------------------------------------------
	// Player's Ferrari (while driving)
	{ 0, 0x0000,  11,  44, Palette_Ferrari },
	{ 0, 0x01e4,  11,  41, Palette_Ferrari },
	{ 0, 0x03a7,  11,  36, Palette_Ferrari },
	{ 0, 0x0533,  11,  44, Palette_Ferrari },
	{ 0, 0x0717,  11,  41, Palette_Ferrari },
	{ 0, 0x08da,  11,  36, Palette_Ferrari },
	{ 0, 0x0a66,  11,  44, Palette_Ferrari },
	{ 0, 0x0c4a,  11,  41, Palette_Ferrari },
	{ 0, 0x0e0d,  11,  36, Palette_Ferrari },

	// Rotating Ferrari.
	{ 0, 0x2489,  17,  41, Palette_Ferrari },
	{ 0, 0x2742,  23,  40, Palette_Ferrari },
	{ 0, 0x2ada,  22,  40, Palette_Ferrari },
	{ 0, 0x2e4a,  17,  41, Palette_Ferrari },

	{ 2, 0x5fba,  19,  33, Palette_Ferrari }, // Ferrari sideways, missing center (intro?)
	{ 2, 0x622d,   7,  38, Palette_Ferrari }, // Door closing.
	{ 2, 0x6337,   7,  38, Palette_Ferrari },
	{ 2, 0x6441,   7,  38, Palette_Ferrari },
	
	// Ending	
	{ 3, 0x0000,  19,  40, Palette_Ferrari },
	{ 3, 0x02f8,   6,  23, Palette_Ferrari },
	{ 3, 0x0382,   5,  23, Palette_Ferrari }, // Door opening
	{ 3, 0x03f5,   5,  25, Palette_Ferrari },
	{ 3, 0x0472,   4,  26, Palette_Ferrari },
	{ 3, 0x04da,   3,  28 },
	{ 3, 0x052e,   8,  27 }, // Interior (passenger door opening).
	{ 3, 0x0606,   7,  27 },
	{ 3, 0x06c3,   6,  27 },
	
	// Ferrari, breaking down (ending A)
	// Possibly different palette
	{ 0, 0x58ef,  22,  44, Palette_Ferrari },
	{ 0, 0x5cb7,  22,  55, Palette_Ferrari },
	{ 0, 0x6171,  22,  40, Palette_Ferrari },
	// Door
	{ 0, 0x64e1,   8,  23, Palette_Ferrari },
	{ 0, 0x6599,   6,  25, Palette_Ferrari },
	
	// Interior
	{ 0, 0x662f,  10,  27 },
	{ 0, 0x673d,   8,  27 },
	{ 0, 0x6815,   7,  27 },
	{ 0, 0x68d2,   7,  27 },
	{ 0, 0x698f,   7,  27 },
	{ 0, 0x6a4c,   7,  27 },

	// Ferrari flipping over (different palette, needs splitting).
	{ 2, 0xb161,  19, 71, Palette_Ferrari2 }, 
	{ 2, 0xb6a6,  19, 99, Palette_Ferrari2 }, 
	{ 2, 0xbdff,  18, 84, Palette_Ferrari2 },
	{ 2, 0xc3e7,  18, 38, Palette_Ferrari2 },
	{ 2, 0xc693,  18, 70, Palette_Ferrari2 },
	{ 2, 0xcb7f,  18, 78, Palette_Ferrari2 },
	{ 2, 0xd0fb,  18, 97, Palette_Ferrari2 },
	// End ferrari

	//---------------------------------------------------------------------------------------------------------------------------------------------------------
	// Guy + interior (rotating)
	{ 2, 0x924f,   3,  12, Palette_Guy }, 
	{ 2, 0x9273,   4,  13, Palette_Guy },
	{ 2, 0x92a7,   6,  14, Palette_Guy },
	{ 2, 0x92fb,   3,   9, Palette_Guy },
	{ 2, 0x931e,   4,  10, Palette_Guy },
	{ 2, 0x9346,   7,  14, Palette_Guy },
	{ 2, 0x93a8,   4,  13, Palette_Guy },
	{ 2, 0x93dc,   4,  13, Palette_Guy },
	{ 2, 0x9410,   7,  14, Palette_Guy }, 
	{ 2, 0x9472,   8,  15, Palette_Guy },
	{ 2, 0x94ea,   6,  15, Palette_Guy },
	{ 2, 0x9550,   4,   9, Palette_Guy },
	{ 2, 0x9574,   4,  15, Palette_Guy },
	{ 2, 0x95b0,   7,  14, Palette_Guy },
	{ 2, 0x9612,   8,  14, Palette_Guy },
	{ 2, 0x9682,   7,  14, Palette_Guy },
	{ 2, 0x96e4,   3,  13, Palette_Girl },
	{ 2, 0x970b,   2,  13, Palette_Girl },
	{ 2, 0x9725,   3,  13, Palette_Girl },
	{ 2, 0x974c,   3,  14, Palette_Girl }, 
	{ 2, 0x9776,   3,   9, Palette_Girl },
	{ 2, 0x9791,   2,  15, Palette_Girl },
	{ 2, 0x97af,   2,  11, Palette_Girl },
	{ 2, 0x97c5,   2,  11, Palette_Girl },
	{ 2, 0x97db,   4,  13, Palette_Guy },
	{ 2, 0x980f,   4,  13, Palette_Guy },
	{ 2, 0x9843,   2,  12, Palette_Guy },
	{ 2, 0x985b,   3,  12, Palette_Girl },
	{ 2, 0x987f,   3,  12, Palette_Girl },
	{ 2, 0x98a3,   3,  14, Palette_Girl },
	{ 2, 0x98cd,   3,  14, Palette_Girl },

	{ 2, 0xd7cd,   3,  16, Palette_Girl }, // Girl pointing
	{ 2, 0xd7fd,   3,  15, Palette_Girl }, 
	{ 2, 0xd82d,   2,  13, Palette_Girl },
	{ 2, 0xd847,   3,  13, Palette_Guy },
	{ 2, 0xd86e,   3,  13, Palette_Guy },
	{ 2, 0xd895,   4,  14, Palette_Girl },
	{ 2, 0xd8cd,   4,  14, Palette_Girl },
	
	{ 2, 0xd905,   3,  17, Palette_Girl }, // Angry
	{ 2, 0xd938,   3,  17, Palette_Girl },
	{ 2, 0xd96b,   3,  17, Palette_Girl },

	// These need a different palette.
	{ 2, 0x8a7f,   5,  13 }, // Guy+girl (in-car)
	{ 2, 0x8acf,   5,  13 }, // Guy+girl (in-car)
	{ 2, 0x8b1f,   5,  13 }, // Guy+girl (in-car)
	{ 2, 0x8b6f,   5,  13 }, // Guy+girl (in-car)
	{ 2, 0x8bb5,   5,  15 }, // Guy + girl pointing (in-car)
	{ 2, 0x8c00,   3,  13 }, // Guy
	{ 2, 0x8c27,   2,  13 }, // Girl
	{ 2, 0x8c99,   7,  26 }, // Guy+girl (in-game, driving, animation 2 frames)
	
	//---------------------------------------------------------------------------------------------------------------------------------------------------------
	// Guy getting out of car, lots of sprites.
	{ 3, 0x2793,   2,  13, Palette_Guy },
	{ 3, 0x27ad,   5,  18, Palette_Guy },
	{ 3, 0x280c,   5,  15, Palette_Guy },
	{ 3, 0x2857,   4,  34, Palette_Guy },
	{ 3, 0x28df,   5,  45, Palette_Guy },
	{ 3, 0x29c0,   4,  49, Palette_Guy },
	{ 3, 0x2a84,   4,  59, Palette_Guy },
	{ 3, 0x2b70,   4,  59, Palette_Guy },
	{ 3, 0x2c5c,   4,  67, Palette_Guy },
	{ 3, 0x2d68,   3,  59, Palette_Guy },
	{ 3, 0x2e19,   4,  59, Palette_Guy },
	{ 3, 0x2f05,   4,  59, Palette_Guy },
	{ 3, 0x2ff1,   4,  59, Palette_Guy },
	{ 3, 0x30dd,   4,  59, Palette_Guy },
	{ 3, 0x31c9,   4,  58, Palette_Guy },
	{ 3, 0x32b9,   4,  57, Palette_Guy },
	{ 3, 0x33a1,   4,  58, Palette_Guy },
	{ 3, 0x348d,   4,  58, Palette_Guy },
	{ 3, 0x3575,   2,  60, Palette_Guy },
	{ 3, 0x35ed,   3,  58, Palette_Guy },
	{ 3, 0x369b,   4,  58, Palette_Guy },
	{ 3, 0x3783,   5,  58, Palette_Guy },
	{ 3, 0x38a5,   5,  58, Palette_Guy },
	{ 3, 0x39c7,   4,  58, Palette_Guy },
	{ 3, 0x3aaf,   4,  58, Palette_Guy },
	{ 3, 0x3b97,   4,  58, Palette_Guy },
	{ 3, 0x3c7f,   5,  58, Palette_Guy },
	{ 3, 0x3da1,   5,  58, Palette_Guy },
	{ 3, 0x3ec3,   8,  22, Palette_Guy },
	{ 3, 0x3f73,   6,  29, Palette_Guy },
	{ 3, 0x4021,   8,  34, Palette_Guy },

	// After crash	
	{ 3, 0x615c,   4,  59, Palette_Guy },
	{ 3, 0x6248,   3,  59, Palette_Guy },
	{ 3, 0x62f9,   3,  59, Palette_Guy },
	{ 3, 0x63aa,   6,  59, Palette_Guy },
	{ 3, 0x650c,   5,  59, Palette_Guy },
	{ 3, 0x6633,   6,  59, Palette_Guy },

	// Getting prize
	{ 2, 0xd99e,   3,  14, Palette_Guy },
	{ 2, 0xd9c8,   3,  57, Palette_Guy },
	{ 2, 0xda73,   3,  57, Palette_Guy },
	{ 2, 0xdb1e,   4,  58, Palette_Guy }, // Taking prize.
	{ 2, 0xdc06,   4,  58, Palette_Guy }, // Taking prize.
	{ 2, 0xdcee,   3,  58, Palette_Guy },
	{ 2, 0xdd9c,   3,  58, Palette_Guy },
	{ 2, 0xde4a,   3,  58, Palette_Guy }, 

	// Guy getting magic lamp.
	{ 3, 0xf850,   4,  58, Palette_Guy }, 
	{ 3, 0xf938,   4,  58, Palette_Guy },
	{ 3, 0xfa20,   4,  58, Palette_Guy },
	{ 3, 0xfb08,   4,  58, Palette_Guy },
	{ 3, 0xfbf0,   4,  58, Palette_Guy },
	{ 3, 0xfcd8,   4,  56, Palette_Guy },
	{ 3, 0xfdb8,   4,  56, Palette_Guy },
	{ 3, 0xfe98,   4,  56, Palette_Guy },
	
	// Guy, tumbling from car.
	{ 0, 0x3ef5,   5,  26, Palette_Guy },
	{ 0, 0x3fa4,   5,  36, Palette_Guy },
	{ 0, 0x405d,   5,  36, Palette_Guy },
	{ 0, 0x413e,   5,  24, Palette_Guy },
	{ 0, 0x41d4,   5,  33, Palette_Guy },
	{ 0, 0x4283,   5,  31, Palette_Guy },
	{ 0, 0x433c,   7,  26, Palette_Guy },
	{ 0, 0x43f2,   7,  18, Palette_Guy },
	{ 0, 0x4470,   5,  24, Palette_Guy },
	{ 0, 0x44e8,   5,  27, Palette_Guy },
	{ 0, 0x456f,   5,  27, Palette_Guy },
	{ 0, 0x45f6,   5,  27, Palette_Guy },
	{ 0, 0x467d,   5,  28, Palette_Guy },
	{ 0, 0x4709,   5,  28, Palette_Guy },

	// Thrown in the air by distracted men.
	{ 3, 0x79d4,   8,  24, Palette_Guy },
	{ 3, 0x7a94,   7,  20, Palette_Guy },
	{ 3, 0x7b20,   5,  35, Palette_Guy },
	{ 3, 0x7bcf,   8,  29, Palette_Guy },
	{ 3, 0x7cb7,   8,   6, Palette_Guy },  // Sunglasses.
	{ 3, 0x7cff,   8,  15, Palette_Guy },
	{ 3, 0x7d77,   9,  15, Palette_Guy },
	// End guy

	//---------------------------------------------------------------------------------------------------------------------------------------------------------
	// Girl getting out of car.	
	{ 3, 0x0765,   2,  12, Palette_Girl },
	{ 3, 0x077d,   3,  16, Palette_Girl },
	{ 3, 0x07ad,   4,  55, Palette_Girl }, // Girl
	{ 3, 0x088d,   4,  55, Palette_Girl },
	{ 3, 0x0969,   4,  55, Palette_Girl },
	{ 3, 0x0a45,   4,  55, Palette_Girl },
	{ 3, 0x0b21,   4,  54, Palette_Girl },
	{ 3, 0x0bfd,   4,  54, Palette_Girl },
	{ 3, 0x0cd9,   4,  54, Palette_Girl },
	{ 3, 0x0db5,   3,  56, Palette_Girl },
	{ 3, 0x0e5d,   3,  56, Palette_Girl },
	{ 3, 0x0f05,   3,  56, Palette_Girl },
	{ 3, 0x0fad,   3,  56, Palette_Girl },
	{ 3, 0x1055,   3,  56, Palette_Girl },
	{ 3, 0x10fd,   3,  53, Palette_Girl }, // With some empty at the bottom. 53 = real height.
	{ 3, 0x11a5,   2,  55, Palette_Girl },
	{ 3, 0x1213,   2,  55, Palette_Girl },
	{ 3, 0x1281,   4,  55, Palette_Girl }, // Pointing
	{ 3, 0x135d,   4,  55, Palette_Girl },
	{ 3, 0x1439,   4,  55, Palette_Girl },

	// After crashing.
	{ 3, 0x5b40,   4,  55, Palette_Girl },
	{ 3, 0x5c1c,   4,  56, Palette_Girl },
	{ 3, 0x5cfc,   4,  56, Palette_Girl },
	{ 3, 0x5ddc,   4,  56, Palette_Girl },
	{ 3, 0x5ebc,   4,  56, Palette_Girl },
	{ 3, 0x5f9c,   4,  56, Palette_Girl },
	{ 3, 0x607c,   4,  56, Palette_Girl },
	
	// Girl taking trophy.
	{ 2, 0xdef8,   3,  55, Palette_Girl },
	{ 2, 0xdf9d,   4,  55, Palette_Girl },
	{ 2, 0xe079,   4,  55, Palette_Girl },
	{ 2, 0xe155,   4,  55, Palette_Girl },
	{ 2, 0xe231,   4,  55, Palette_Girl },
	{ 2, 0xe30d,   4,  55, Palette_Girl },
	{ 2, 0xe3e9,   4,  55, Palette_Girl },
	{ 2, 0xe4c5,   4,  55, Palette_Girl },
	{ 2, 0xe5a1,   4,  55, Palette_Girl },
	{ 2, 0xe67d,   4,  55, Palette_Girl },
	{ 2, 0xe759,   4,  55, Palette_Girl },
	{ 2, 0xe835,   4,  55, Palette_Girl },
	{ 2, 0xe911,   3,  55, Palette_Girl },
	{ 2, 0xe9b6,   3,  55, Palette_Girl },
	{ 2, 0xea5b,   4,  55, Palette_Girl },
	
	// Girl, tumbling from car.	
	{ 0, 0x47a9,   5,  25, Palette_Girl },
	{ 0, 0x487b,   5,  24, Palette_Girl },
	{ 0, 0x4939,   5,  33, Palette_Girl },
	{ 0, 0x4a1f,   5,  21, Palette_Girl },
	{ 0, 0x4aa6,   5,  37, Palette_Girl },
	{ 0, 0x4b64,   5,  32, Palette_Girl },
	{ 0, 0x4c27,   5,  32, Palette_Girl },
	{ 0, 0x4cc7,   6,  22, Palette_Girl },
	{ 0, 0x4d4b,   5,  11, Palette_Girl },
	{ 0, 0x4d82,   4,  24, Palette_Girl },
	{ 0, 0x4de2,   4,  28, Palette_Girl },
	{ 0, 0x4e52,   4,  28, Palette_Girl },
	{ 0, 0x4ec2,   4,  28, Palette_Girl },
	// End girl.

	//---------------------------------------------------------------------------------------------------------------------------------------------------------
	// Trophy girl (no palette yet).
	{ 3, 0x1515,   5,  55, Palette_TrophyGirl },
	{ 3, 0x1628,   5,  55, Palette_TrophyGirl },
	{ 3, 0x173b,   5,  55, Palette_TrophyGirl },
	{ 3, 0x184e,   5,  55, Palette_TrophyGirl },
	{ 3, 0x1961,   5,  55, Palette_TrophyGirl },
	{ 3, 0x1a74,   5,  55, Palette_TrophyGirl },
	{ 3, 0x1b87,   4,  56, Palette_TrophyGirl },
	{ 3, 0x1c67,   5,  56, Palette_TrophyGirl },
	{ 3, 0x1d7f,   5,  56, Palette_TrophyGirl },
	{ 3, 0x1e97,   4,  56, Palette_TrophyGirl },
	{ 3, 0x1f77,   3,  56, Palette_TrophyGirl },
	{ 3, 0x201f,   3,  56, Palette_TrophyGirl },
	{ 3, 0x20c7,   4,  56, Palette_TrophyGirl },
	{ 3, 0x21a7,   4,  56, Palette_TrophyGirl },
	{ 3, 0x2287,   4,  55, Palette_TrophyGirl },
	{ 3, 0x2367,   4,  55, Palette_TrophyGirl },
	{ 3, 0x2447,   4,  55, Palette_TrophyGirl },
	{ 3, 0x2527,   4,  55, Palette_TrophyGirl },
	{ 3, 0x2607,   3,  56, Palette_TrophyGirl },
	{ 3, 0x26af,   4,  57, Palette_TrophyGirl },
	
	{ 2, 0xeb37,   3,  55, Palette_TrophyGirl }, // Pointing.
	{ 2, 0xebdc,   3,  55, Palette_TrophyGirl }, 

	//---------------------------------------------------------------------------------------------------------------------------------------------------------
	 // Bikini girl.
	{ 3, 0xde53,   4,  57, Palette_BikiniGirl },
	{ 3, 0xdf37,   4,  57, Palette_BikiniGirl },
	{ 3, 0xe01b,   4,  57, Palette_BikiniGirl },
	{ 3, 0xe0ff,   4,  57, Palette_BikiniGirl },
	{ 3, 0xe1e3,   4,  57, Palette_BikiniGirl },
	{ 3, 0xe2c7,   4,  57, Palette_BikiniGirl },
	{ 3, 0xe3ab,   3,  57, Palette_BikiniGirl },

	//---------------------------------------------------------------------------------------------------------------------------------------------------------
	// Magic lamp guy
	{ 3, 0xe70e,   7,  76 }, // Man with fez saying 'magic lamp!'
	{ 3, 0xe922,   4,  58 },
	{ 3, 0xea0a,   4,  58 },
	{ 3, 0xeaf2,   4,  58 }, 
	{ 3, 0xebda,   4,  58 }, 
	{ 3, 0xecc2,   4,  58 },
	{ 3, 0xedaa,   4,  58 },
	{ 3, 0xee92,   4,  58 },
	{ 3, 0xef7a,   4,  58 },

	{ 3, 0xf062,   5,  58 },
	{ 3, 0xf184,   4,  58 },
	{ 3, 0xf26c,   3,  58 },
	{ 3, 0xf31a,   3,  58 },
	{ 3, 0xf3c8,   4,  58 },
	{ 3, 0xf4b0,   3,  58 },
	{ 3, 0xf55e,   3,  58 },
	{ 3, 0xf60c,   4,  58 },
	{ 3, 0xf6f4,   3,  58 },
	{ 3, 0xf7a2,   3,  58 },
	
	{ 3, 0xe456,  12,  58 }, // Magic lamp girls.

//
	
	
	
	
	
	





	
	// Porsche (5 zoom levels)
	{ 0, 0x0f99,   8,  43, Palette_Porsche },
//	{ 0, 0x10f1,   4,  21, Palette_Porsche },
//	{ 0, 0x1145,   3,  10, Palette_Porsche },
//	{ 0, 0x1163,   2,   5, Palette_Porsche },
//	{ 0, 0x116d,   1,   2, Palette_Porsche },
	{ 0, 0x116f,   8,  45, Palette_Porsche },
//	{ 0, 0x12d7,   4,  22, Palette_Porsche },
//	{ 0, 0x132f,   3,  11, Palette_Porsche },
//	{ 0, 0x1350,   2,   5, Palette_Porsche },
//	{ 0, 0x135a,   1,   2, Palette_Porsche },
	{ 0, 0x135c,   9,  43, Palette_Porsche },
//	{ 0, 0x14df,   4,  24, Palette_Porsche },
//	{ 0, 0x153f,   3,  12, Palette_Porsche },
//	{ 0, 0x1563,   2,   6, Palette_Porsche },
//	{ 0, 0x156f,   1,   3, Palette_Porsche },
	{ 0, 0x1572,   9,  45, Palette_Porsche },
//	{ 0, 0x1707,   5,  22, Palette_Porsche },
//	{ 0, 0x1775,   3,  11, Palette_Porsche },
//	{ 0, 0x1796,   2,   5, Palette_Porsche },
//	{ 0, 0x17a0,   1,   2, Palette_Porsche },
	{ 0, 0x17a2,  10,  43, Palette_Porsche },
//	{ 0, 0x1950,   5,  21, Palette_Porsche },
//	{ 0, 0x19b9,   3,  10, Palette_Porsche },
//	{ 0, 0x19d7,   2,   5, Palette_Porsche },
//	{ 0, 0x19e1,   1,   2, Palette_Porsche },
	{ 0, 0x19e3,  10,  45, Palette_Porsche },
//	{ 0, 0x1ba5,   5,  22, Palette_Porsche },
//	{ 0, 0x1c13,   3,  11, Palette_Porsche },
//	{ 0, 0x1c34,   2,   5, Palette_Porsche },
//	{ 0, 0x1c3e,   1,   2, Palette_Porsche },

	// Pickup (5 zoom levels)
	{ 0, 0x1c40,   6,  36 },
//	{ 0, 0x1d18,   3,  18 },
//	{ 0, 0x1d4e,   2,   9 },
//	{ 0, 0x1d60,   1,   4 }, // Edited manually.
//	{ 0, 0x1d64,   1,   2 },
	{ 0, 0x1d66,   6,  38 },
//	{ 0, 0x1e4a,   3,  19 },
//	{ 0, 0x1e83,   2,   9 },
//	{ 0, 0x1e95,   1,   4 },
//	{ 0, 0x1e99,   1,   2 }, // Edited manually.
	{ 0, 0x1e9b,   7,  36 },
//	{ 0, 0x1f97,   4,  18 },
//	{ 0, 0x1fdf,   2,   9 },
//	{ 0, 0x1ff1,   2,   4 }, // Edited manually.
//	{ 0, 0x1ff9,   1,   2 },
	{ 0, 0x1ffb,   7,  38 },
//	{ 0, 0x2105,   4,  19 },
//	{ 0, 0x2151,   2,   9 },
//	{ 0, 0x2163,   2,   4 }, // Edited manually.
//	{ 0, 0x216b,   1,   2 },
	{ 0, 0x216d,   8,  36 },
//	{ 0, 0x228d,   4,  18 },
//	{ 0, 0x22d5,   2,   9 },
//	{ 0, 0x22e7,   2,   4 }, // Edited manually.
//	{ 0, 0x22ef,   1,   2 },
	{ 0, 0x22f1,   8,  38 },
//	{ 0, 0x2421,   4,  19 },
//	{ 0, 0x246d,   2,   9 },
//	{ 0, 0x247f,   2,   4 }, // Edited manually.
//	{ 0, 0x2487,   1,   2 },

	// Convertible.
	{ 0, 0x3103,   7,  32 },
	{ 0, 0x31e3,   4,  16 },
	{ 0, 0x3223,   2,  12 },
	{ 0, 0x323b,   1,   2 },
	{ 0, 0x323d,   7,  33 },
	{ 0, 0x3324,   4,  16 },
	{ 0, 0x3364,   2,  12 },
	{ 0, 0x337c,   1,   2 },
	{ 0, 0x337e,   7,  32 },
	{ 0, 0x345e,   4,  16 },
	{ 0, 0x349e,   2,  12 },
	{ 0, 0x34b6,   1,   2 },
	{ 0, 0x34b8,   7,  33 },
	{ 0, 0x359f,   4,  16 },
	{ 0, 0x35df,   2,  12 },
	{ 0, 0x35f7,   1,   2 },
	{ 0, 0x35f9,   8,  32 },
	{ 0, 0x36f9,   4,  16 },
	{ 0, 0x3739,   3,   8 },
	{ 0, 0x3751,   2,   4 },
	{ 0, 0x3759,   1,   2 },
	{ 0, 0x375b,   8,  33 },
	{ 0, 0x3863,   4,  16 },
	{ 0, 0x38a3,   3,   8 },
	{ 0, 0x38bb,   2,   4 },
	{ 0, 0x38c3,   1,   2 },
	{ 0, 0x38c5,   6,  55 },
	{ 0, 0x3a0f,   3,  27 },
	{ 0, 0x3a60,   2,  13 },
	{ 0, 0x3a7a,   1,   9 },
	{ 0, 0x3a83,   7,  55 },
	{ 0, 0x3c04,   4,  27 },
	{ 0, 0x3c70,   2,  19 },
	{ 0, 0x3c96,   1,   3 },
	{ 0, 0x3c99,   8,  55 },
	{ 0, 0x3e51,   4,  27 },
	{ 0, 0x3ebd,   2,  19 },
	{ 0, 0x3ee3,   1,   3 },
//	{ 0, 0x3ee6,   2,   1 },
//	{ 0, 0x3ee8,   1,   1 },
//	{ 0, 0x3ee9,   2,   2 },
//	{ 0, 0x3eed,   1,   1 },
//	{ 0, 0x3eee,   2,   2 },
//	{ 0, 0x3ef2,   1,   1 },
//	{ 0, 0x3ef3,   2,   1 },


	// Motorcycle.
	{ 0, 0x4f36,   4,  40 },
	{ 0, 0x4fd2,   2,  20 },
	{ 0, 0x4ffa,   1,  17 },
	{ 0, 0x500b,   4,  39 },
	{ 0, 0x50a7,   2,  28 },
	{ 0, 0x50df,   1,   6 },
	{ 0, 0x50e5,   4,  38 },
	{ 0, 0x517d,   2,  28 },
	{ 0, 0x51b5,   1,   6 },
	{ 0, 0x51bb,   6,  31 },
	{ 0, 0x5275,   3,  15 },
	{ 0, 0x52a2,   2,   7 },
	{ 0, 0x52b0,   1,   4 },
	{ 0, 0x52b4,   7,  31 },
	{ 0, 0x538d,   4,  15 },
	{ 0, 0x53c9,   2,  10 },
	{ 0, 0x53dd,   1,   1 },
	{ 0, 0x53de,   8,  31 },
	{ 0, 0x54d6,   4,  15 },
	{ 0, 0x5512,   3,   7 },
	{ 0, 0x5527,   2,   3 },
	{ 0, 0x552d,   1,   1 },
	{ 0, 0x552e,   6,  35 },
	{ 0, 0x5600,   3,  17 },
	{ 0, 0x5633,   2,   8 },
	{ 0, 0x5643,   1,   6 },
	{ 0, 0x5649,   7,  35 },
	{ 0, 0x573e,   4,  17 },
	{ 0, 0x5782,   2,  12 },
	{ 0, 0x579a,   1,   2 },
	{ 0, 0x579c,   7,  35 },
	{ 0, 0x5891,   4,  17 },
	{ 0, 0x58d5,   2,  12 },
	{ 0, 0x58ed,   1,   2 },

	{ 0, 0x6b09,   4,  48 },
	{ 0, 0x6bc9,   5,  11 },
	{ 0, 0x6c00,   2,  25 },
	{ 0, 0x6c32,   8,  71 },
	{ 0, 0x6e6a,  10,  72 },
	{ 0, 0x716c,  10,  34 },
	{ 0, 0x72c0,   6,  20 },
	{ 0, 0x7350,   6,  25 },
	{ 0, 0x73e6,   9,  51 },
	{ 0, 0x75b1,   7,  55 },
	{ 0, 0x7732,  10,  15 },
	{ 0, 0x7804,  10,  24 },
	{ 0, 0x78f4,   7,  24 },
	{ 0, 0x79aa,   7,  22 },
	{ 0, 0x7a44,  10,  22 },
	{ 0, 0x7b20,   8,  37 },
	{ 0, 0x7c48,   6,  65 },
	{ 0, 0x7dce,   7,  18 },
	{ 0, 0x7e4c,   8,   7 },
//	{ 0, 0x7e84, 396,   1 }, // Fixme: empty?

	// Big tree.
	{ 0, 0x8000,  16, 124, Palette_Foliage1 },
	{ 0, 0x87d0,   9,  63, Palette_Foliage1 },
	{ 0, 0x8a07,   5,  31, Palette_Foliage1 },
	{ 0, 0x8aa2,   3,  15, Palette_Foliage1 },
	{ 0, 0x8acf,   2,   7, Palette_Foliage1 },
	
	{ 0, 0x8add,   9, 160 },
	{ 0, 0x907d,   5,  80 },
	{ 0, 0x920d,   3,  40 },
	{ 0, 0x9285,   2,  20 },
	{ 0, 0x92ad,   1,  10 },
	{ 0, 0x92b7,  12,  56 },
	{ 0, 0x9557,   6,  28 },
	{ 0, 0x95ff,   3,  14 },
	{ 0, 0x9629,   2,   7 },
	{ 0, 0x9637,   1,   3 },
	{ 0, 0x963a,  17,  53 },
	{ 0, 0x99bf,   9,  26 },
	{ 0, 0x9aa9,   5,  13 },
	{ 0, 0x9aea,   3,   6 },
	{ 0, 0x9afc,   2,   3 },
	
	// Palm tree, 
	{ 0, 0x9b02,  11, 168, Palette_Palmtree },
	{ 0, 0xa23a,   6,  84, Palette_Palmtree },
	{ 0, 0xa432,   3,  42, Palette_Palmtree },
	{ 0, 0xa4b0,   2,  21, Palette_Palmtree },
	{ 0, 0xa4da,   1,  10, Palette_Palmtree },
	{ 0, 0xa4e4,  16, 126 },
	{ 0, 0xacc4,   8,  63 },
	{ 0, 0xaebc,   5,  31 },
	{ 0, 0xaf57,   3,  15 },
	{ 0, 0xaf84,   2,   7 },
	{ 0, 0xaf92,  15, 132 },
	{ 0, 0xb74e,   8,  66 },
	{ 0, 0xb95e,   4,  33 },
	{ 0, 0xb9e2,   3,  16 },
	{ 0, 0xba12,   2,   8 },
	{ 0, 0xba22,  17,  79 },
	{ 0, 0xbf61,   9,  39 },
	{ 0, 0xc0c0,   5,  19 },
	{ 0, 0xc11f,   3,   9 },
	{ 0, 0xc13a,   1,   2 },
	{ 0, 0xc13c,   2,   1 },
	{ 0, 0xc13e,  13,  47 },
	{ 0, 0xc3a1,   7,  23 },
	{ 0, 0xc442,   4,  11 },
	{ 0, 0xc46e,   2,   5 },
	{ 0, 0xc478,   1,   2 },
	{ 0, 0xc47a,   7,  91 },
	{ 0, 0xc6f7,   4,  45 },
	{ 0, 0xc7ab,   2,  22 },
	{ 0, 0xc7d7,   1,   9 },
	{ 0, 0xc7e0,   2,   1 },
	{ 0, 0xc7e2,   1,   5 },
	{ 0, 0xc7e7,   6,  76 },
	{ 0, 0xc9af,   3,  38 },
	{ 0, 0xca21,   2,  20 },
	{ 0, 0xca49,   1,  11 },
	{ 0, 0xca54,   8,  44 },
	{ 0, 0xcbb4,   4,  22 },
	{ 0, 0xcc0c,   2,  11 },
	{ 0, 0xcc22,   1,   3 },
	{ 0, 0xcc25,   2,   1 },
	{ 0, 0xcc27,   1,   2 },
	{ 0, 0xcc29,   6,  70 },
	{ 0, 0xcdcd,   3,  35 },
	{ 0, 0xce36,   2,  19 },
	{ 0, 0xce5c,   1,   8 },
	{ 0, 0xce64,   9,  26 },
	{ 0, 0xcf4e,   5,  13 },
	{ 0, 0xcf8f,   3,   6 },
	{ 0, 0xcfa1,   2,   3 },
	{ 0, 0xcfa7,   1,   1 },
	{ 0, 0xcfa8,  14,  43 },
	{ 0, 0xd202,   7,  21 },
	{ 0, 0xd295,   4,  10 },
	{ 0, 0xd2bd,   2,   6 },
	// Last one.
	{ 0, 0xd2c9,  31,  32 /* 271 */ },
	{ 0, 0xd6a9,  31,  30 /* 271 */ },
	{ 0, 0xda4b,  31,  28 /* 271 */ },
	{ 0, 0xddaf,  31,  26 /* 271 */ },
	{ 0, 0xe0d5,  31, 255 /* 271 */ },
	
	// Bank 1
	{ 1, 0x0000,  24,   8 },
	{ 1, 0x00c0,  12,   4 },
	{ 1, 0x00f0,   6,   2 },
	{ 1, 0x00fc,   3,   1 },
	{ 1, 0x00ff,   1,   1 },
	{ 1, 0x0100,  22,   8 },
	{ 1, 0x01b0,  11,   4 },
	{ 1, 0x01dc,   6,   2 },
	{ 1, 0x01e8,   3,   1 },
	{ 1, 0x01eb,   1,   1 },
	{ 1, 0x01ec,  20,   8 },
	{ 1, 0x028c,  10,   4 },
	{ 1, 0x02b4,   5,   2 },
	{ 1, 0x02be,   3,   1 },
	{ 1, 0x02c1,   1,   1 },
	{ 1, 0x02c2,  18,   8 },
	{ 1, 0x0352,   9,   4 },
	{ 1, 0x0376,   5,   2 },
	{ 1, 0x0380,   3,   1 },
	{ 1, 0x0383,   1,   1 },
	{ 1, 0x0384,  16,   8 },
	{ 1, 0x0404,   8,   4 },
	{ 1, 0x0424,   4,   2 },
	{ 1, 0x042c,   2,   1 },
	{ 1, 0x042e,   1,   1 },
	{ 1, 0x042f,  14,   8 },
	{ 1, 0x049f,   7,   4 },
	{ 1, 0x04bb,   4,   2 },
	{ 1, 0x04c3,   2,   1 },
	{ 1, 0x04c5,   1,   1 },
	{ 1, 0x04c6,  12,   8 },
	{ 1, 0x0526,   6,   4 },
	{ 1, 0x053e,   3,   2 },
	{ 1, 0x0544,   2,   1 },
	{ 1, 0x0546,   1,   1 },
	{ 1, 0x0547,  10,   8 },
	{ 1, 0x0597,   5,   4 },
	{ 1, 0x05ab,   3,   2 },
	{ 1, 0x05b1,   2,   1 },
	{ 1, 0x05b3,   1,   1 },
	{ 1, 0x05b4,   8,   8 },
	{ 1, 0x05f4,   4,   4 },
	{ 1, 0x0604,   2,   2 },
	{ 1, 0x0608,   1,   2 },
	{ 1, 0x060a,   6,   8 },
	{ 1, 0x063a,   3,   4 },
	{ 1, 0x0646,   2,   2 },
	{ 1, 0x064a,   1,   2 },
	{ 1, 0x064c,   4,   8 },
	{ 1, 0x066c,   2,   6 },
	{ 1, 0x0678,   1,   2 },
	{ 1, 0x067a,   2,   8 },
	{ 1, 0x068a,   1,   8 },
	{ 1, 0x0692,  12,  43 },
	{ 1, 0x0896,   6,  21 },
	{ 1, 0x0914,   4,  10 },
	{ 1, 0x093c,   2,   5 },
	{ 1, 0x0946,   1,   2 },
	{ 1, 0x0948,  31,  32 }, // 272 },
	{ 1, 0x0d28,  31,  30 }, // 272 },
	{ 1, 0x10ca,  31,  28 }, // 272 },
	{ 1, 0x142e,  31,  26 }, // 272 },
	{ 1, 0x1754,  31,  24 }, // 272 },
	{ 1, 0x1a3c,  31, 255 }, // 272 },

	{ 1, 0x2a38,  11, 122, Palette_Stone1 },
	{ 1, 0x2f76,   6,  61, Palette_Stone1 },
	{ 1, 0x30e4,   3,  30, Palette_Stone1 },
	{ 1, 0x313e,   2,  15, Palette_Stone1 },
	{ 1, 0x315c,   1,   7, Palette_Stone2 },
	{ 1, 0x3163,  30,  48, Palette_Stone2 },
	{ 1, 0x3703,  15,  24, Palette_Stone2 },
	{ 1, 0x386b,   8,  12, Palette_Stone2 },
	{ 1, 0x38cb,   4,   6, Palette_Stone2 },
	{ 1, 0x38e3,   3,   3, Palette_Stone2 },
	{ 1, 0x38ec,  29,  32 }, // 272 },
	{ 1, 0x3c8c,  29,  30 }, // 272 },
	{ 1, 0x3ff2,  29,  28 }, // 272 },
	{ 1, 0x431e,  29,  26 }, // 272 },
	{ 1, 0x4610,  29,  24 }, // 272 },
	{ 1, 0x48c8,  29, 255 }, // 272 },
	{ 1, 0x57bc,  10,   7 },
	{ 1, 0x5802,   5,   3 },
	{ 1, 0x5811,   3,   1 },
	{ 1, 0x5814,   2,   1 },
	{ 1, 0x5816,   1,   1 },
	{ 1, 0x5817,  10,  16 },
	{ 1, 0x58b7,   5,   8 },
	{ 1, 0x58df,   3,   4 },
	{ 1, 0x58eb,   2,   2 },
	{ 1, 0x58ef,   1,   1 },
//	{ 1, 0x58f0,   2,   1 },
//	{ 1, 0x58f2,   1,   3 },
//	{ 1, 0x58f5,   2,   2 },
//	{ 1, 0x58f9,   1,   3 },
//	{ 1, 0x58fc,   2,   2 },
//	{ 1, 0x5900,   1,   3 },
//	{ 1, 0x5903,   2,   2 },
//	{ 1, 0x5907,   1,   3 },
//	{ 1, 0x590a,   2,   2 },
//	{ 1, 0x590e,   1,   3 },
//	{ 1, 0x5911,   2,   2 },
//	{ 1, 0x5915,   1,   3 },
//	{ 1, 0x5918,   2,   2 },
//	{ 1, 0x591c,   1,   3 },
//	{ 1, 0x591f,   2,   2 },
//	{ 1, 0x5923,   1,   3 },
//	{ 1, 0x5926,   2,   1 },
	{ 1, 0x5928,   7,  12 },
//	{ 1, 0x597c,   2,   1 },
//	{ 1, 0x597e,   1,   3 },
//	{ 1, 0x5981,   2,   2 },
//	{ 1, 0x5985,   1,   3 },
//	{ 1, 0x5988,   2,   2 },
//	{ 1, 0x598c,   1,   3 },
//	{ 1, 0x598f,   2,   2 },
//	{ 1, 0x5993,   1,   3 },
//	{ 1, 0x5996,   2,   2 },
//	{ 1, 0x599a,   1,   3 },
//	{ 1, 0x599d,   2,   1 },
	{ 1, 0x599f,   7,  15 },
//	{ 1, 0x5a08,   2,   1 },
//	{ 1, 0x5a0a,   1,   3 },
//	{ 1, 0x5a0d,   2,   2 },
//	{ 1, 0x5a11,   1,   3 },
//	{ 1, 0x5a14,   2,   2 },
//	{ 1, 0x5a18,   1,   3 },
//	{ 1, 0x5a1b,   2,   1 },
	{ 1, 0x5a1d,   7,  17 },
//	{ 1, 0x5a94,   2,   1 },
//	{ 1, 0x5a96,   1,   3 },
//	{ 1, 0x5a99,   2,   2 },
//	{ 1, 0x5a9d,   1,   3 },
//	{ 1, 0x5aa0,   2,   2 },
//	{ 1, 0x5aa4,   1,   3 },
//	{ 1, 0x5aa7,   2,   2 },
//	{ 1, 0x5aab,   1,   3 },
//	{ 1, 0x5aae,   2,   2 },
//	{ 1, 0x5ab2,   1,   3 },
//	{ 1, 0x5ab5,   2,   2 },
//	{ 1, 0x5ab9,   1,   3 },
//	{ 1, 0x5abc,   2,   2 },
//	{ 1, 0x5ac0,   1,   3 },
//	{ 1, 0x5ac3,   2,   2 },
//	{ 1, 0x5ac7,   1,   3 },
//	{ 1, 0x5aca,   2,   2 },
//	{ 1, 0x5ace,   1,   3 },
//	{ 1, 0x5ad1,   2,   1 },
	{ 1, 0x5ad3,   7,  11 },
	{ 1, 0x5b20,  28,  64 },
	{ 1, 0x6220,  28,  56 },
	{ 1, 0x6840,  28,  48 },
	{ 1, 0x6d80,  28,  40 },
	{ 1, 0x71e0,  28, 255 },
	{ 1, 0x7af4, 1307,   1 },
	
	{ 1, 0x8000,  15,  73 },
	{ 1, 0x8447,   8,  36 },
	{ 1, 0x8567,   4,  18 },
	{ 1, 0x85af,   3,   9 },
	{ 1, 0x85ca,   2,   4 },
	{ 1, 0x85d2,   5,  27 },
	{ 1, 0x8659,   3,  13 },
	{ 1, 0x8680,   2,   6 },
	{ 1, 0x868c,   1,   4 },
	{ 1, 0x8690,  14, 103 },
	{ 1, 0x8c32,   7,  51 },
	{ 1, 0x8d97,   4,  25 },
	{ 1, 0x8dfb,   2,  18 },
	{ 1, 0x8e1f,  28,  51 },
	{ 1, 0x93b3,  14,  25 },
	{ 1, 0x9511,   8,  12 },
	{ 1, 0x9571,   4,   6 },
	{ 1, 0x9589,   2,   3 },
	{ 1, 0x958f,  13, 101 },
	{ 1, 0x9ab0,   7,  50 },
	{ 1, 0x9c0e,   4,  25 },
	{ 1, 0x9c72,   2,  12 },
	{ 1, 0x9c8a,   1,   6 },
	{ 1, 0x9c90,   8, 112 },
	{ 1, 0xa010,   4,  56 },
	{ 1, 0xa0f0,   3,  28 },
	{ 1, 0xa144,   2,  14 },
	{ 1, 0xa160,   1,   7 },
	{ 1, 0xa167,  15,  80 },
	{ 1, 0xa617,   8,  40 },
	{ 1, 0xa757,   4,  20 },
	{ 1, 0xa7a7,   3,  10 },
	{ 1, 0xa7c5,   2,   5 },
	{ 1, 0xa7cf,  17,  48 },
	{ 1, 0xaaff,   9,  24 },
	{ 1, 0xabd7,   5,  12 },
	{ 1, 0xac13,   3,   6 },
	{ 1, 0xac25,   2,   3 },
	{ 1, 0xac2b,  16, 127 },
	{ 1, 0xb41b,   8,  63 },
	{ 1, 0xb613,   5,  31 },
	{ 1, 0xb6ae,   3,  15 },
	{ 1, 0xb6db,   2,   7 },
	{ 1, 0xb6e9,  19,  92 },
	{ 1, 0xbdbd,  10,  46 },
	{ 1, 0xbf89,   5,  23 },
	{ 1, 0xbffc,   3,  11 },
	{ 1, 0xc01d,   2,   5 },
	{ 1, 0xc027,  22, 178 },
	{ 1, 0xcf73,  11,  89 },
	{ 1, 0xd346,   6,  44 },
	{ 1, 0xd44e,   3,  22 },
	{ 1, 0xd490,   2,  11 },
	{ 1, 0xd4a6,  30,  61 },
	{ 1, 0xdbcc,  15,  30 },
	{ 1, 0xdd8e,   8,  15 },
	{ 1, 0xde06,   4,   7 },
	{ 1, 0xde22,   2,   3 },
	{ 1, 0xde28,  30,  61 },
	{ 1, 0xe54e,  15,  30 },
	{ 1, 0xe710,   8,  15 },
	{ 1, 0xe788,   4,   7 },
	{ 1, 0xe7a4,   3,   3 },
	{ 1, 0xe7ad,   8,  87 },
	{ 1, 0xea65,   6, 105 },
	{ 1, 0xecdb,   9, 128 },
	{ 1, 0xf15b,   5,  64 },
	{ 1, 0xf29b,   3,  32 },
	{ 1, 0xf2fb,   2,  16 },
	{ 1, 0xf31b,   1,   8 },
	{ 1, 0xf323,   2,   7 },
	{ 1, 0xf331,   3,  19 },
	{ 1, 0xf36a,   4,  29 },
	{ 1, 0xf3de,   9, 177 },
	{ 1, 0xfa17,   7,  32 },
	{ 1, 0xfaf7,   4,  10 },

	//---------------------------------------------------------------------------------------------------------------------------------------------------------
	// Bank 2
	{ 2, 0x0000,  28,  46 },
//	{ 2, 0x0508,  14,  23 },
//	{ 2, 0x064a,   8,  11 },
//	{ 2, 0x06a2,   4,   5 },
//	{ 2, 0x06b6,   2,   2 },
	{ 2, 0x06ba,  10, 128 },
//	{ 2, 0x0bba,   5,  64 },
//	{ 2, 0x0cfa,   3,  32 },
//	{ 2, 0x0d5a,   2,  16 },
//	{ 2, 0x0d7a,   1,   8 },
	{ 2, 0x0d82,   4, 112 },
//	{ 2, 0x0f42,   2,  56 },
//	{ 2, 0x0fb2,   2,  28 },
//	{ 2, 0x0fea,   1,   6 },
//	{ 2, 0x0ff0,   2,   1 },
	{ 2, 0x0ff2,   1,  13 },
	{ 2, 0x0fff,  15,  64 },
	{ 2, 0x13bf,   8,  32 },
	{ 2, 0x14bf,   4,  16 },
	{ 2, 0x14ff,   3,   8 },
	{ 2, 0x1517,   2,   4 },
	{ 2, 0x151f,  16,  86 },
	{ 2, 0x1a7f,   8,  43 },
	{ 2, 0x1bd7,   5,  21 },
	{ 2, 0x1c40,   3,  10 },
	{ 2, 0x1c5e,   2,   5 },
	{ 2, 0x1c68,   4,  28 },
	{ 2, 0x1cd8,   2,  14 },
	{ 2, 0x1cf4,   1,  11 },
	{ 2, 0x1cff,  24,  65 },
	{ 2, 0x2317,  12,  32 },
	{ 2, 0x2497,   7,  16 },
	{ 2, 0x2507,   4,   8 },
	{ 2, 0x2527,   2,   4 },
	{ 2, 0x252f,  15, 140 },
	{ 2, 0x2d63,   8,  70 },
	{ 2, 0x2f93,   4,  34 },
	{ 2, 0x301b,   2,  27 },
	{ 2, 0x3051,  10, 164 },
	{ 2, 0x36b9,   5,  82 },
	{ 2, 0x3853,   3,  41 },
	{ 2, 0x38ce,   2,  20 },
	{ 2, 0x38f6,   1,  10 },
	{ 2, 0x3900,  15,  42 },
	{ 2, 0x3b76,   8,  21 },
	{ 2, 0x3c1e,   4,  10 },
	{ 2, 0x3c46,   3,   5 },
	{ 2, 0x3c55,   2,   2 },
	{ 2, 0x3c59,   6,  67 },
	{ 2, 0x3deb,   3,  33 },
	{ 2, 0x3e4e,   2,  16 },
	{ 2, 0x3e6e,   1,  12 },
	{ 2, 0x3e7a,   6,  15 },
	{ 2, 0x3ed4,   3,   7 },
	{ 2, 0x3ee9,   2,   3 },
	{ 2, 0x3eef,   1,   2 },
	{ 2, 0x3ef1,   3,   8 },
	{ 2, 0x3f09,   2,   4 },
	{ 2, 0x3f11,   1,   4 },
	{ 2, 0x3f15,  15,  55 },
	{ 2, 0x424e,   8,  27 },
	{ 2, 0x4326,   4,  13 },
	{ 2, 0x435a,   3,   6 },
	{ 2, 0x436c,   2,   3 },
	{ 2, 0x4372,   8, 170 },
	{ 2, 0x48c2,   4,  85 },
	{ 2, 0x4a16,   3,  42 },
	{ 2, 0x4a94,   2,  21 },
	{ 2, 0x4abe,   1,  10 },

	// Flag guy
	{ 2, 0x4ac8,   5,  97 },
	{ 2, 0x4cad,   7,  93 },
	{ 2, 0x4f38,   8,  65 },
	{ 2, 0x5140,   8,  66 },
	{ 2, 0x5350,   8,  76 },
	{ 2, 0x55b0,   7,  83 },
	{ 2, 0x57f5,   6,  66 },
	{ 2, 0x5981,   7,  62 },
	{ 2, 0x5b33,   8,  63 },
	{ 2, 0x5d2b,   6,  87 },
	

	{ 2, 0x654b,   1,  96 },
	{ 2, 0x65ab,   3,  48 },
	{ 2, 0x663b,   5,  63 },
	{ 2, 0x6776,   3,  31 },
	{ 2, 0x67d3,   2,  15 },
	{ 2, 0x67f1,   1,  10 },
	{ 2, 0x67fb,   5,  34 },
	{ 2, 0x68a5,   3,  17 },
	{ 2, 0x68d8,   2,   8 },
	{ 2, 0x68e8,   1,   6 },
	{ 2, 0x68ee,   4,  41 },
	{ 2, 0x6992,   2,  30 },
	{ 2, 0x69ce,   1,   1 },
	{ 2, 0x69cf,   2,   1 },
	{ 2, 0x69d1,   1,   4 },
	{ 2, 0x69d5,   6,  27 },
	{ 2, 0x6a77,   3,  13 },
	{ 2, 0x6a9e,   2,   6 },
	{ 2, 0x6aaa,   1,   4 },
	{ 2, 0x6aae,   8,  40 },
	{ 2, 0x6bee,   4,  20 },
	{ 2, 0x6c3e,   2,  15 },
	{ 2, 0x6c5c,   1,   2 },
	{ 2, 0x6c5e,  11,  40 },
	{ 2, 0x6e16,   5,  21 },
	{ 2, 0x6e7f,  13,  22 },
	{ 2, 0x6f9d,   5,  22 },
	{ 2, 0x700b,  12,  34 },
	{ 2, 0x71a3,   5,  21 },
	{ 2, 0x720c,   3,  70 }, // A B C D E (for map)
	{ 2, 0x72de,   9, 173 },
	{ 2, 0x78f3,   7, 126 },
	{ 2, 0x7c65,   6,  71 },
	{ 2, 0x7e0f, 498,   1 },
	
	{ 2, 0x8001,   1, 213 },
	{ 2, 0x80d6,   2, 112 },
	{ 2, 0x81b6,   1, 104 },
	{ 2, 0x821e,   3, 112 },
	{ 2, 0x836e,   2,  56 },
	{ 2, 0x83de,   1,  49 },
	{ 2, 0x840f,   2, 112 },
	{ 2, 0x84ef,   1, 105 },
	{ 2, 0x8558,   2, 112 },
	{ 2, 0x8638,   1, 105 },
	{ 2, 0x86a1,  56,   1 },
	{ 2, 0x86d9,   7,  78 }, // Surfer
	{ 2, 0x88fb,   4,  43 },
	{ 2, 0x89a7,   2,  31 },
	{ 2, 0x89e5,   1,   5 },
	{ 2, 0x89ea,   3,  31 },
	{ 2, 0x8a47,   2,  15 },
	{ 2, 0x8a65,   1,  11 },
	{ 2, 0x8a70,   2,   1 },
	{ 2, 0x8a72,   1,   1 },
	{ 2, 0x8a73,   2,   2 },
	{ 2, 0x8a77,   1,   1 },
	{ 2, 0x8a78,   2,   2 },
	{ 2, 0x8a7c,   1,   1 },
	{ 2, 0x8a7d,   2,   1 },

	
	
	{ 2, 0x8c41,   3,  20 },
	{ 2, 0x8c7d,   2,  10 },
	{ 2, 0x8c91,   1,   8 },

	{ 2, 0x8d59,  10,  29 }, // Mud splatter.
	{ 2, 0x8e99,  10,  30 }, // Mud splatter.
	{ 2, 0x8fd9,  10,  30 }, // Mud splatter.
	{ 2, 0x9123,  10,  30 }, // Mud splatter.
	

	{ 2, 0x98f7,  23,  88 }, // Outrun logo background.
	{ 2, 0xa0df,  12,  44 },
	{ 2, 0xa2ef,   6,  22 },
	{ 2, 0xa373,   4,  11 },
	{ 2, 0xa39f,   2,   5 },
	{ 2, 0xa3a9,  18,  36 }, // Out run logo foreground.
	{ 2, 0xa631,   9,  18 },
	{ 2, 0xa6d3,   5,   9 },
	{ 2, 0xa700,   3,   4 },
	{ 2, 0xa70c,   4,   1 },
	{ 2, 0xa710,   1,   3 },
	{ 2, 0xa713,   2,   1 },
	{ 2, 0xa715,   7,  56 }, // Logo palm tree.
	{ 2, 0xa89d,   4,  28 },
	{ 2, 0xa90d,   2,  14 },
	{ 2, 0xa929,   1,   2 },
	{ 2, 0xa92b,   2,   1 },
	{ 2, 0xa92d,   1,   6 },
	{ 2, 0xa933,   7,  57 },
	{ 2, 0xaac2,   4,  28 },
	{ 2, 0xab32,   2,  14 },
	{ 2, 0xab4e,   1,   2 },
	{ 2, 0xab50,   2,   1 },
	{ 2, 0xab52,   1,   6 },
	{ 2, 0xab58,   7,  57 },
	{ 2, 0xace7,   4,  28 },
	{ 2, 0xad57,   2,  14 },
	{ 2, 0xad73,   1,   2 },
	{ 2, 0xad75,   2,   1 },
	{ 2, 0xad77,   1,   6 },
	{ 2, 0xad7d,  13,  25 }, // Logo road
	{ 2, 0xaec2,   7,  12 },
	{ 2, 0xaf16,   4,   6 },
	{ 2, 0xaf2e,   2,   3 },
	{ 2, 0xaf34,   1,   1 },
	{ 2, 0xaf35,   9,  39 }, // Logo car.
	{ 2, 0xb094,   5,  19 },
	{ 2, 0xb0f3,   3,   9 },
	{ 2, 0xb10e,   2,   4 },
	{ 2, 0xb116,   1,   2 },
	{ 2, 0xb118,   3,   6 },
	{ 2, 0xb12a,   2,   3 },
	{ 2, 0xb130,   1,   1 },
	{ 2, 0xb131,   2,   1 },
	{ 2, 0xb133,   3,   6 },
	{ 2, 0xb145,   2,   3 },
	{ 2, 0xb14b,  22,   1 },

	
	
	
	{ 2, 0xec81,  31,  32 },
	{ 2, 0xf061,  16,  16 },
	{ 2, 0xf161,   8,   8 },
	{ 2, 0xf1a1,   5,   4 },
	{ 2, 0xf1b5,   3,   2 },
	{ 2, 0xf1bb,   5,  47 },
	{ 2, 0xf2a6,   2,   1 },
	{ 2, 0xf2a8,   1,   1 },
	{ 2, 0xf2a9,   2,   2 },
	{ 2, 0xf2ad,   1,   1 },
	{ 2, 0xf2ae,   2,   2 },
	{ 2, 0xf2b2,   1,   1 },
	{ 2, 0xf2b3,   2,   1 },
	{ 2, 0xf2b5,   5,   2 },
	{ 2, 0xf2bf,   2,   1 },
	{ 2, 0xf2c1,   1,   1 },
	{ 2, 0xf2c2,   2,   1 },
	{ 2, 0xf2c4,   5,   1 },
	{ 2, 0xf2c9,   3,  33 },
	{ 2, 0xf32c,   1,  12 },
	{ 2, 0xf338,   4, 192 },
	{ 2, 0xf638,   2, 144 },
	{ 2, 0xf758,   1,  24 },
	{ 2, 0xf770,   9,   1 },
	{ 2, 0xf779,   2,   1 },
	{ 2, 0xf77b,   1,   5 },
	{ 2, 0xf780,   2,   2 },
	{ 2, 0xf784,   1,   5 },
	{ 2, 0xf789,   2,   2 },
	{ 2, 0xf78d,   1,   5 },
	{ 2, 0xf792,   2,   1 },
	{ 2, 0xf794,   9,   2 },
	{ 2, 0xf7a6,   2,   1 },
	{ 2, 0xf7a8,   1,   5 },
	{ 2, 0xf7ad,   2,   2 },
	{ 2, 0xf7b1,   1,   5 },
	{ 2, 0xf7b6,   2,   1 },
	{ 2, 0xf7b8,   9, 136 },

#endif
#if 1 // Bank 3 mostly complete.
	//---------------------------------------------------------------------------------------------------------------------------------------------------------
	// Bank 3

	
	// Radio stuff. First, frequencies.
	{ 3, 0x4131,   4,   7 }, // 69.2
	{ 3, 0x414d,   4,   7 },
	{ 3, 0x4169,   4,   7 },
	// Radio button.
	{ 3, 0x4185,   4,  25, Palette_Radio_Knob },
	{ 3, 0x41e9,   4,  25, Palette_Radio_Knob },
	{ 3, 0x424d,   4,  25, Palette_Radio_Knob },

	// Radio VU meter, one color for each dot.
	{ 3, 0x42b1,   4,   5 },
	{ 3, 0x42c5,  17,  30, Palette_Radio }, // Actual radio panel.
	{ 3, 0x44c3,  18,  62, Palette_Radio_Arm }, // Arm x3
	{ 3, 0x491f,  18,  58, Palette_Radio_Arm },
	{ 3, 0x4d33,  17,  64, Palette_Radio_Arm },
	
	// Splash with shadow.
	{ 3, 0x5173,   9,  31 },
	{ 3, 0x528a,  14,  29 },
	{ 3, 0x5420,  13,  31 },
	{ 3, 0x55c9,  11,  29 },
	{ 3, 0x5708,  14,  31 },
	{ 3, 0x58ba,  14,  30 },
	
	{ 3, 0x5a5e,   6,   8 },
	{ 3, 0x5a8e,   3,   4 },
	{ 3, 0x5a9a,   2,   2 },
	{ 3, 0x5a9e,   1,   2 },
	{ 3, 0x5aa0,   7,   8 },
	{ 3, 0x5ad8,   4,   4 },
	{ 3, 0x5ae8,   2,   1 },
	{ 3, 0x5aea,   4,   1 },
	{ 3, 0x5aee,   7,   8 },
	{ 3, 0x5b26,   4,   4 },
	{ 3, 0x5b36,   2,   1 },
	{ 3, 0x5b38,   1,   1 }, 
	

	// Guys throwing player
	{ 3, 0x6795,   9, 49 },
	{ 3, 0x694e,   9, 53 },
	{ 3, 0x6b2b,   9, 71 }, 
	{ 3, 0x6daa,   9, 48 }, 
	{ 3, 0x6f5a,   9, 54 }, 
	{ 3, 0x7140,   9, 71 }, 
	{ 3, 0x73bf,   9, 49 }, 
	{ 3, 0x7578,   9, 53 }, 
	{ 3, 0x7755,   9, 71 }, 
	
	// GOAL banner (2 parts)
	{ 3, 0x8000,  30,  62 },
	{ 3, 0x8744,  15,  31 },
	{ 3, 0x8915,   8,  15 },
	{ 3, 0x898d,   4,   7 },
	{ 3, 0x89a9,   2,   3 },
	{ 3, 0x89af,  30,  62 },
	{ 3, 0x90f3,  15,  31 },
	{ 3, 0x92c4,   8,  15 },
	{ 3, 0x933c,   4,   7 },
	{ 3, 0x9358,   3,   3 },
	
	// Splash
	{ 3, 0x936b,   5,  17 },
	{ 3, 0x93c0,   5,  21 },
	{ 3, 0x9429,   5,  20 },
	{ 3, 0x948d,   5,  17 },
	{ 3, 0x94f1,   5,  12 },
	
	{ 3, 0x952d,   4, 61 },
	{ 3, 0x9621,   4, 61 },
	{ 3, 0x9715,   5,  35 },
	{ 3, 0x97c4,   4,  60 },
	{ 3, 0x98b4,   4,  62 },
	{ 3, 0x99ac,   3,  54 },
	{ 3, 0x9a4e,   4,  61 },
	{ 3, 0x9b42,   3,  63 },
	
	// Clouds wide! (could use all of them at once?)
	{ 3, 0x9bff,  31, 32 },
	{ 3, 0x9fdf,  31, 28 },
	{ 3, 0xa343,  31, 24 },
	{ 3, 0xa62b,  31, 20 },
	{ 3, 0xa897,  31, 16 },
	{ 3, 0xaa87,  31, 12 },
	{ 3, 0xabfb,  31,  8 },
	{ 3, 0xacf3,  31,  4 },
	{ 3, 0xad6f,  31,  2 },

	// Natives with spears.
	{ 3, 0xadad,  31,  42 },
	{ 3, 0xb2c3,  16,  21 },
	{ 3, 0xb413,   8,  10 },
	{ 3, 0xb463,   4,   5 },
	{ 3, 0xb477,   2,   2 },

	// Small clouds? Snow?
	{ 3, 0xb47b,  31,  16 },
	{ 3, 0xb66b,  31,  15 },
	{ 3, 0xb83c,  31,  14 },
	{ 3, 0xb9ee,  31,  13 },
	{ 3, 0xbb81,  31,  12 },
	{ 3, 0xbcf5,  31,  11 },
	{ 3, 0xbe4a,  31,  10 },
	{ 3, 0xbf80,  31,   9 },
	{ 3, 0xc097,  31,   8 },
	{ 3, 0xc18f,  31,   7 },
	{ 3, 0xc268,  31,   6 },
//	{ 3, 0xc322,  31,   4 }, // Unclear where to split. Also ugly.
//	{ 3, 0xc39e,  31,  30 },
//	{ 3, 0xc496,  31,   3 },
	
	// 3 guys walking 
	{ 3, 0xc4f3,   9,  58 },
	{ 3, 0xc6fd,   9,  58 },
	{ 3, 0xc907,  10,  58 },
	{ 3, 0xcb4b,   9,  58 },
	{ 3, 0xcd55,   9,  58 },
	{ 3, 0xcf5f,  10,  58 },
	{ 3, 0xd1a3,   9,  58 },
	{ 3, 0xd3ad,   9,  58 },
	{ 3, 0xd5b7,  10,  58 },
	{ 3, 0xd7fb,   9,  58 },
	{ 3, 0xda05,   9,  58 },
	{ 3, 0xdc0f,  10,  58 },
	
	

#endif // end bank 3
};




// Ok weet je wat
// data uit outrun (130000):
// 078d 5a90 07b6  733d a33d 0100 0000 5a91

// This copy will be filled by the user parameters, and then copied into the active sprite list.
SpriteData g_userSpriteData = { 0 };

// Fills in the above structure with settings from one of the entries found by walking the sprite ROM banks.
void SetupSpriteDefaults (SpriteData* pSprite, uint16_t spriteIdx)
{
	const SpriteInfo* pSpriteInfo = &GameSprites[spriteIdx];
	pSprite->Bank            = pSpriteInfo->BankIdx;
	pSprite->BankOffset      = pSpriteInfo->BankOffset;
	pSprite->Pitch           = pSpriteInfo->Pitch;
	pSprite->Height          = pSpriteInfo->Height - 1;
	
	pSprite->DrawTopToBottom = 1;
	pSprite->FlipHorizontal  = 1; // ?
	pSprite->DrawLeftToRight = 1;
	
	pSprite->PaletteIdx      = pSpriteInfo->PaletteIdx;
}

uint16_t g_spriteIdx = 0;
uint16_t const kNumSpriteInfos = sizeof(GameSprites)/sizeof(SpriteInfo);

void PrintSpriteValues (const SpriteData* pSprite, uint16_t spriteIdx)
{
	const uint8_t xpos = 14;
	uint8_t ypos = 3;
	TEXT_SetColor (TEXT_Purple);
	
	TEXT_GotoXY (xpos-1, ypos);
	TEXT_WriteHex (spriteIdx, 3, ' ');

	// Bank idx.	
	ypos += 2;
	TEXT_GotoXY (xpos, ypos);
	TEXT_WriteHex (pSprite->Bank, 2, ' ');
	
	// Bank offset.
	ypos += 2;
	TEXT_GotoXY (xpos-2, ypos);
	TEXT_WriteHex (pSprite->BankOffset, 4, ' ');
	
	// Pitch
	ypos += 2;
	TEXT_GotoXY (xpos, ypos);
	TEXT_WriteHex ((uint8_t)pSprite->Pitch, 2, ' ');
	
	// Height
	ypos += 2;
	TEXT_GotoXY (xpos, ypos);
	TEXT_WriteHex (pSprite->Height, 2, ' ');

	// Height
	ypos += 6;
	TEXT_GotoXY (xpos, ypos);
	TEXT_WriteHex (pSprite->PaletteIdx, 2, ' ');

	
	// Print next sprite address (temp).
	TEXT_GotoXY (2, 23);
	TEXT_Write ("Next address ");
	TEXT_WriteHex ((uint16_t)(pSprite->BankOffset + pSprite->Pitch * (pSprite->Height + 1)), 4, '0');
}

bool ChangeSprite (InputButton button, uint16_t param)
{
	bool spriteChanged = false;
	if (button == Button_Brake && g_spriteIdx > 0)
	{
		g_spriteIdx--;
		spriteChanged = true;
	} 
	else if (button == Button_Accelerate && (g_spriteIdx+1) < kNumSpriteInfos)
	{
		g_spriteIdx++;
		spriteChanged = true;
	}
	
	if (spriteChanged)
	{
		SetupSpriteDefaults (&g_userSpriteData, g_spriteIdx);
		PrintSpriteValues (&g_userSpriteData, g_spriteIdx);
	}
	return false;
}

bool ChangePitch (InputButton button, uint16_t param) 
{
	if (g_userSpriteData.Pitch > -64 && button == Button_Brake)
		g_userSpriteData.Pitch--;
	else if (g_userSpriteData.Pitch < 63 && button == Button_Accelerate)
		g_userSpriteData.Pitch++;
	
	PrintSpriteValues (&g_userSpriteData, g_spriteIdx);
	return false; 
}

bool ChangeHeight (InputButton button, uint16_t param) 
{
	if (g_userSpriteData.Height > 0 && button == Button_Brake)
		g_userSpriteData.Height--;
	else if (g_userSpriteData.Height < 0xff && button == Button_Accelerate)
		g_userSpriteData.Height++;
	
	PrintSpriteValues (&g_userSpriteData, g_spriteIdx);
	return false; 
}

bool ChangePalette (InputButton button, uint16_t param) 
{
	if (g_userSpriteData.PaletteIdx > 0 && button == Button_Brake)
		g_userSpriteData.PaletteIdx--;
	else if (g_userSpriteData.PaletteIdx < 127 && button == Button_Accelerate)
		g_userSpriteData.PaletteIdx++;
	
	PrintSpriteValues (&g_userSpriteData, g_spriteIdx);
	return false; 
}

bool FlipHorizontal (InputButton button, uint16_t param) 
{
//	g_userSpriteData.DrawLeftToRight ^= 1; // This one just really draws left to right. No address change required.
	g_userSpriteData.DrawTopToBottom ^= 1;
	PrintSpriteValues (&g_userSpriteData, g_spriteIdx);
	return false;
}

bool ToggleShadows (InputButton button, uint16_t param) 
{
//	g_userSpriteData.DrawLeftToRight ^= 1; // This one just really draws left to right. No address change required.
	g_userSpriteData.EnableShadows ^= 1;
	PrintSpriteValues (&g_userSpriteData, g_spriteIdx);
	return false;
}


bool Dummy (InputButton button, uint16_t param) { return false; }

static const MenuItem s_menuItems[] = 
{
	{ "Sprite", ChangeSprite, 0 },
	{ "Bank", Dummy, 0 },
	{ "Offset", Dummy, 0 },
	{ "Pitch",  ChangePitch, 0 },
	{ "Height", ChangeHeight, 0 },
	{ "Flip H", FlipHorizontal, 0 },
	{ "Flip V", Dummy, 0 },
	{ "Shadows", ToggleShadows, 0 },
	{ "Palette", ChangePalette, 0 },
};

void main ()
{
	HW_Init (HWINIT_Default, 0x321); // Very dark blue.
	TEXT_InitDefaultPalette ();
	PPI_Write_Register (PPI_WRITE_PORT_C, PPI_C_SG1, PPI_C_SG1);

	// Print header.
	TEXT_GotoXY (14,1);
	TEXT_SetColor (TEXT_Yellow);
	TEXT_Write ("sprite sample");
	
	// Initialize input helper.
	InputState inputState;
	INPUT_Init (&inputState);

	// Initialize menu, this draws the initial menu state.
	MenuState menuState;
	MENU_Init (&menuState, s_menuItems, sizeof(s_menuItems)/sizeof(MenuItem), 2, 3, -1);

	// Set up some default sprite palettes.
	for (uint8_t i=0; i<(sizeof(CarPalette)/sizeof(uint16_t)); i++)
		PALETTE_SetColor16 (0x800+i, CarPalette[i]);

	// Globals.	
	g_userSpriteData.Top = 0x100 + 24;
	g_userSpriteData.Left = 0x155 + 20 - 32;
	g_userSpriteData.HorizontalZoomFactor = 0x200;
	g_userSpriteData.VerticalZoomFactor = 0x200;

	SetupSpriteDefaults (&g_userSpriteData, g_spriteIdx);
	PrintSpriteValues (&g_userSpriteData, g_spriteIdx);

	// Misschien ff een flinke clear.
	/*
      ; Wacht op 60800 (set by interrupt 4).
    > 7DDA tst.w $60800.l
      7DE0 bge $7dda
      ; 60800 is nu 0xffff
      7DE2 cmpi.w #-$1, $60800.l       ; compare 60800 met -1.
      7DEA beq $7df4
     (7DEE addq.w #1, $60ba4.l)        ; increase $60ba4
      7DF4 move.w #$1, $60800.l        ; 0x60800 = 1.
      7DFC tst.w $140060.l             ; watchdog.
      7E02 rts                         ; terug naar 975E

    ; wait for 60800 again.
    9762 tst.w $60800.l  ; flags??
    9768 bne $9762
    
    ; 60800 is nu 0.
    ; Clear sprite RAM (disable all 128 sprites)
    976A move.l #$c0000000, D1         ; D1 = 0xc0000000 = end of list.
    9770 moveq #$0, D2                 ; D2 = 0
    9772 lea $130000.l, A0             ; A0 = 0x130000
    9778 move.w #$7f, D0               ; D0 = 0x7f
  > 977C move.l D1, (A0)+              ; write 0xc00000000 to 130000+
    977E move.l D2, (A0)+              ; write 0x000000000 to 130004+
    9780 move.l D2, (A0)+              ; write 0x000000000 to 130008+
    9782 move.l D2, (A0)+              ; write 0x000000000 to 13000c+
    9784 dbra D0, $977c                ; loop 0x80 times (128 sprites)
     
    9788 move.l #$55555555, $1307fc    ; Set the last DWORD in sprite ram to 0x55555555. Why?
    9792 not.w $140070.l               ; Write to sprite chip (!!!) -- flip sprite bank.
    9798 bsr $7dda                     ; Wacht op adres 0x68000 (= wacht op interrupt).
    ; 68000 = 1.

  > 979C tst.w $60800.l                ; wait for 0x68000 (again - different?).
    97A2 bne $979c
    ; 68000 = 0

    ; Clear all 128 sprites, again. Double buffering?
    97A4 lea $130000.l, A0             ; A0 = 0x130000
    97AA move.w #$7f, D0               ; D0 = 0x7f
  > 97AE move.l D1, (A0)+	       ; write 0xc0000000 to 130000+
    97B0 move.l D2, (A0)+              ; write 0 to 130004+
    97B2 move.l D2, (A0)+              ; write 0 to 130008+
    97B4 move.l D2, (A0)+              ; write 0 to 13000c+
    97B6 dbra D2, $97ae                ; repeat for all 128 sprites.
    97BA move.l #$aaaaaaaa, $1307fc    ; Set the last DWORD in sprite ram to 0xaaaaaaaa. Why? Scratch space?
    97C4 bclr #$7, $60b6f.l            ; Clear bit 7 of 0x60b6f

    97CC bsr $7dda                     ; Wait for 0x68000 (interrupt)
    ; 68000 = 1
  > 97D0 tst.w $60800.l
    97D6 bne $97d0                     ; wait again.
    ; 68000 = 0
    97D8 bset #$7, $60bb6f.l           ; Set bit 7 of 0x60b6f
    97E0 rts                           ; back to 8356

  8356 bsr $84c0 
	*/
	
	IRQ4_Wait ();
	volatile uint32_t* pDest = (volatile uint32_t*)&SpriteList[0];
	for (uint16_t i=0; i<128; i++)
	{
		*pDest++ = 0xc0000000;
		*pDest++ = 0;
	}
	*(--pDest) = 0x55555555; // ?
	
	__asm__("not.w 0x140070.l");
	IRQ4_Wait ();
	
	// Do it again.
	pDest = (volatile uint32_t*)&SpriteList[0];
	for (uint16_t i=0; i<128; i++)
	{
		*pDest++ = 0xc0000000;
		*pDest++ = 0;
	}
	*(--pDest) = 0xaaaaaaaa; // ?

	
	for (;;)
	{
		static uint8_t x = 0;
		x++;
		IRQ4_Wait ();

		// Poll input.
		INPUT_Update (&inputState);
		MENU_Update (&menuState, &inputState);

		SpriteData userSprite = g_userSpriteData; // copy.
		SpriteData* pSprite = &userSprite; // Fill this thing.
		
/*
goed; 11ed2 boel adressen. 
eerste is 0xFACE ya rly

och zucht

// 0x11ED2: Table of Sprite Addresses for Hardware. Contains:
//
// 5 x 10 bytes. One block for each sprite size lookup. 
// The exact sprite is selected using the ozoom_lookup.h table.
//
// + 0 : [Byte] Unused
// + 1 : [Byte] Width Helper Lookup  [Offsets into 0x20000 (the width and height table)]
// + 2 : [Byte] Line Data Width
// + 3 : [Byte] Height Helper Lookup [Offsets into 0x20000 (the width and height table)]
// + 4 : [Byte] Line Data Height
// + 5 : [Byte] Sprite Pitch (should be *2)
// + 7 : [Byte] Sprite Bank
// + 8 : [Word] Offset Within Sprite Bank 

0080 007c 0010 0000 8000   
                    ~~~~ bank offset
0048 003e 0009 0000 87d0

00f8 001f 001f 0002 ec81 
*/		
		
		// Okay dit is een ferrari.
		// Prima zo.
//		((uint16_t*)pSprite)[0] = 0x1b5;
//		((uint16_t*)pSprite)[1] = 0x1e4;


//		pSprite->Bank = 0; // Hmm, bank size of 0x40000.

//		pSprite->BankOffset = 0x533; // 0x1e4; // moet dit keer iets ofzo. ik denk *4.
//		pSprite->Pitch = 11; // Wat dan, b? B koeien?	44 pixelts?
//		pSprite->Height = 43; // Laten we zeggen 44.

		
		pSprite->Top = 0x100 + 24;

		
		//
		// region()->bytes() / 0x40000 ahja, een bank is dan zo 0x40000 = 256k. Dan moet die offset * 4.
		// de pitch is ook * 4.
		// dus 1e4 => sprite @ 790.
		
		// Each 32-bit word read from sprite ROM holds eight 4-bit pixels. 
		
		// Ik snap die pitch niet.
		
//		((uint16_t*)pSprite)[2] = 0x1735 ;

		pSprite->Left = 0x155;
		
//		((uint16_t*)pSprite)[3] = 0x3200;
		
		pSprite->Priority = 3;

//		((uint16_t*)pSprite)[4] = 0xe200;
		//pSprite->DrawTopToBottom = 1;
//		pSprite->FlipHorizontal = 1; // 0 = flip?
//		pSprite->DrawLeftToRight = 1; // This needs adjustment of the starting X pos.
		pSprite->HorizontalZoomFactor = 0x200;
		pSprite->VerticalZoomFactor = 0x200;
		

		
		
		
		
		
//		((uint16_t*)pSprite)[5] = 0x2812;
		
		// Hm ja welke dan.
		//pSprite->PaletteIdx = 0; // Fixme: don't have palette. Maar soit.
		
		
//		pSprite->HorizontalZoomFactor = 0x12;
				
//		((uint16_t*)pSprite)[6] = 0x0;
//		((uint16_t*)pSprite)[7] = 0;
		
		
		// Okay, die zoom verneukt niet de boel met pitch. Goddank.
		//pSprite->Height = 64; // Die weet ik niet, maar dat maakt ook niet zoveel uit.
		
//		pSprite->DrawTopToBottom = pSprite->DrawLeftToRight = 1;

//		pSprite->Left = SPRITE_LEFT + 10; //-20;
//		pSprite->PaletteIdx = 0;




//		*pSprite = g_userSpriteData;

		// Now write to buffer.
		memcpy (&SpriteList[0], pSprite, sizeof(SpriteData));
		*((uint16_t*)&SpriteList[1]) = 0xffff; // TERMINATE
		
		IRQ4_Wait ();
		SPRITE_SwapBuffers ();
	}
}
