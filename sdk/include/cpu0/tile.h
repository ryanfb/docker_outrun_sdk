/*******************************************************************************************
 *
 *  System 16B-style tilemaps
 *
 *  16 total pages
 *  Column/rowscroll enabled via bits in text layer
 *  Alternate tilemap support
 *
 *  Tile format:
 *      Bits               Usage
 *      p------- --------  Tile priority versus sprites
 *      -??----- --------  Unknown
 *      ---ccccc cc------  Tile color palette
 *      ---nnnnn nnnnnnnn  Tile index
 *
 *  Text format:
 *      Bits               Usage
 *      p------- --------  Tile priority versus sprites
 *      -???---- --------  Unknown
 *      ----ccc- --------  Tile color palette
 *      -------n nnnnnnnn  Tile index
 *
 *  Alternate tile format:
 *      Bits               Usage
 *      p------- --------  Tile priority versus sprites
 *      -??----- --------  Unknown
 *      ----cccc ccc-----  Tile color palette
 *      ---nnnnn nnnnnnnn  Tile index
 *
 *  Alternate text format:
 *      Bits               Usage
 *      p------- --------  Tile priority versus sprites
 *      -???---- --------  Unknown
 *      -----ccc --------  Tile color palette
 *      -------- nnnnnnnn  Tile index
 *
 *  Text RAM:
 *      Offset   Bits               Usage
 *      E80      aaaabbbb ccccdddd  Foreground tilemap page select
 *      E82      aaaabbbb ccccdddd  Background tilemap page select
 *      E84      aaaabbbb ccccdddd  Alternate foreground tilemap page select
 *      E86      aaaabbbb ccccdddd  Alternate background tilemap page select
 *      E90      c------- --------  Foreground tilemap column scroll enable
 *               -------v vvvvvvvv  Foreground tilemap vertical scroll
 *      E92      c------- --------  Background tilemap column scroll enable
 *               -------v vvvvvvvv  Background tilemap vertical scroll
 *      E94      -------v vvvvvvvv  Alternate foreground tilemap vertical scroll
 *      E96      -------v vvvvvvvv  Alternate background tilemap vertical scroll
 *      E98      r------- --------  Foreground tilemap row scroll enable
 *               ------hh hhhhhhhh  Foreground tilemap horizontal scroll
 *      E9A      r------- --------  Background tilemap row scroll enable
 *               ------hh hhhhhhhh  Background tilemap horizontal scroll
 *      E9C      ------hh hhhhhhhh  Alternate foreground tilemap horizontal scroll
 *      E9E      ------hh hhhhhhhh  Alternate background tilemap horizontal scroll
 *      F16-F3F  -------- vvvvvvvv  Foreground tilemap per-16-pixel-column vertical scroll
 *      F56-F7F  -------- vvvvvvvv  Background tilemap per-16-pixel-column vertical scroll
 *      F80-FB7  a------- --------  Foreground tilemap per-8-pixel-row alternate tilemap enable
 *               -------h hhhhhhhh  Foreground tilemap per-8-pixel-row horizontal scroll
 *      FC0-FF7  a------- --------  Background tilemap per-8-pixel-row alternate tilemap enable
 *               -------h hhhhhhhh  Background tilemap per-8-pixel-row horizontal scroll
 *
 *******************************************************************************************/

#ifndef __TILE_H__
#define __TILE_H__

#include "maincpu.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" 
{
#endif // __cplusplus

// Tile page dimensions, in 8x8 tiles.
#define TILE_PAGE_WIDTH 64
#define TILE_PAGE_HEIGHT 32

// Screen dimensions, in 8x8 tiles.
#define TILE_PAGE_VISIBLE_ROWS 28
#define TILE_PAGE_VISIBLE_COLUMNS 40

// Tile map control registers are in text RAM.
#define TILE_REGISTER_BASE ((uint16_t*)0x110E80)

// Scrolling will delay to the right or down, so an increasing scroll value scrolls to the right or down.
// When the scroll position is set to (0,0):
// Pages : DDDDCCCCBBBBAAAA
// +----------+---+------+---- -
// |B         |A  |      |B
// |          |   | VIEW |
// |          |   |      |
// |          |   +------+
// +----------+----------+---- -
// |D         |C

typedef struct
{
	struct TILE_PAGES
	{
		union
		{	
			struct 
			{
				uint16_t Page_3 : 4;
				uint16_t Page_2 : 4;
				uint16_t Page_1 : 4;
				uint16_t Page_0 : 4;
			};
			uint16_t Pages;
		};
	} 	
	ForegroundMapPages, // E80
	BackgroundMapPages, // E82
	AltForegroundMapPages, // E84
	AltBackgroundMapPages; // E86
		
	struct { uint16_t __unused__[4]; };
	
	struct TILE_YSCROLL
	{
		uint16_t ColumnScrollEnable : 1;
		uint16_t : 6;
		uint16_t Y : 9;
	}
	ForegroundScrollY, // E90
	BackgroundScrollY, // E92
	AltForegroundScrollY, // E94
	AltBackgroundScrollY; // E96
	
	struct TILE_XSCROLL
	{
		uint16_t RowScrollEnable : 1;
		uint16_t : 5;
		uint16_t X : 10;
	}
	ForegroundScrollX, // E98
	BackgroundScrollX, // E9A
	AltForegroundScrollX, // E9C
	AltBackgroundScrollX; // E9E
	
	struct { uint16_t __unused_2__[0x3b]; };
	
	// F16..F3F
	struct TILE_COLUMN
	{
		uint16_t : 8;
		uint16_t VerticalScroll : 8;
	} ForegroundColumns[21]; 
		
	// F40.
	uint16_t __unused_3__[11];
	
	// F56..F7F
	struct TILE_COLUMN BackgroundColumns[21];
	
	struct TILE_ROW
	{
		uint16_t AlternateTileMapEnable : 1;
		uint16_t : 6;
		uint16_t HorizontalScroll : 9;
	} ForegroundRows[TILE_PAGE_VISIBLE_ROWS];
	
	uint16_t __unused_4[4];

	struct TILE_ROW BackgroundRows[TILE_PAGE_VISIBLE_ROWS];
	
} TILE_REGS;

#define TileRegisters (*((volatile TILE_REGS*)TILE_REGISTER_BASE))

// Fills one of the 16 4kb tile pages.
void TILE_FillPage (uint8_t pageIdx, uint16_t tileIdx);

// Returns a write pointer to the specified tilemap page.
static inline uint16_t* TILE_GetPagePtr (uint8_t pageIdx)
{
	return (TILE_RAM_BASE + (((uint16_t)(pageIdx&0xf)) << 11));
}

// Clears one tilemap page and selects it for the foreground and background layers.
// Also resets all tile registers.
void TILE_Reset ();

#ifdef __cplusplus
} // extern "C"
#endif // __cplusplus

#endif // __TILE_H__
