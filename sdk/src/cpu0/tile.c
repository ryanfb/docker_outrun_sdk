#include "tile.h"

// Fills one of the 16 4kb tile pages.
void TILE_FillPage (uint8_t pageIdx, uint16_t tileIdx)
{
	uint32_t tileFill = tileIdx | (((unsigned int)tileIdx) << 16);
	uint32_t* pDest = (uint32_t*)(TILE_RAM_BASE + (((uint32_t)(pageIdx&0xf)) << 11));
	for (uint16_t i=0; i<1024; i++)
		*pDest++ = tileFill;
}

// Clears one tilemap page and selects it for the foreground and background layers.
void TILE_Reset ()
{
	// Reset all registers to zero.
	for (uint16_t i=0; i<(0x178 >> 1); i++)
		TILE_REGISTER_BASE[i] = 0;

	// Fill one page with empty tiles. Take page 0 to match our completely clear registers.
	TILE_FillPage (0, 0x20);
}
