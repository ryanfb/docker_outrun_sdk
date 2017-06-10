#include <tile.h>
#include "tileunpack.h"
#include "palette.h"

// Fill a single tile map with zeroes.
void ClearTileMap (uint16_t* pDest)
{
	for (uint16_t x = 0; x<((TILE_PAGE_WIDTH*TILE_PAGE_HEIGHT)>>1); x++)
		((uint32_t*)pDest)[x] = 0;
}

// Unpack without clearing.
void UnpackTileMap (uint16_t* pDest, const uint16_t* pSrc)
{
	uint16_t* pBottom = pDest + ((TILE_PAGE_HEIGHT-1)*TILE_PAGE_WIDTH); // Start at the bottom.
	for (uint8_t x=0; x<TILE_PAGE_WIDTH; x++)
	{
		uint16_t count = *pSrc++;
		
		if (count == (uint16_t)-1)
			break; // Done.
		
		uint16_t* pWrite = pBottom;
		for (uint8_t y=0; y<count; y++)
		{
			*pWrite = (*pSrc++); // | 0x8000;
			pWrite -= TILE_PAGE_WIDTH; // Move up.
		}
		
		pBottom++; // Next column.
	}
}

// Sequentially unpacks multiple tile maps. Also clears the pages.
void UnpackTileMaps (uint8_t firstDestPage, const uint16_t** ppSrcPages, uint8_t nPages)
{
	for (uint8_t i=0; i<nPages; i++)
	{
		uint16_t* pDest = TILE_GetPagePtr (firstDestPage);
		ClearTileMap (pDest);
		UnpackTileMap (pDest, ppSrcPages[i]);
		++firstDestPage;
	}
}

// Unpacks multiple pages, sets up associated palette, all from a single structure.
void UnpackTileGraphics (uint8_t firstDestPage, const TileGraphics* pSrcData)
{
	// Unpack tile pages.
	UnpackTileMaps (firstDestPage, pSrcData->ppPackedTilePages, pSrcData->NumTilePages);
	
	// Set up appropriate colors.
	uint16_t col = pSrcData->PaletteStartDest;
	uint16_t srcIdx = 0;
	for (uint16_t i=0; i<pSrcData->PaletteCountDest; i++)
	{
		PALETTE_SetColor16(col++, pSrcData->pPaletteData[srcIdx]);
		srcIdx++;
		if (srcIdx == pSrcData->PaletteCountSrc)
			srcIdx = 0;
	}
}
