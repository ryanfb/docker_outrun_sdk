#ifndef __TILEUNPACK_H__
#define __TILEUNPACK_H__

#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus

// Basic tile 'unpacker'. The format is quite unsophisticated, but reduces a fair amount of the size of the cloud and hill style tile maps in Out Run.
// The data is stored as bottom-first columns, prefixed with the column length (up to 32). Both length and tile data are stored as 16 bit (big endian) words.
// A count of -1 (0xffff) prematurely ends a page. Otherwise the unpacking normally finishes after 64 columns.

// Fill a single tile map with zeroes.
void ClearTileMap (uint16_t* pDest);

// Unpack without clearing.
void UnpackTileMap (uint16_t* pDest, const uint16_t* pSrc);

// Sequentially unpacks multiple tile maps. Also clears the pages.
void UnpackTileMaps (uint8_t firstDestPage, const uint16_t** ppSrcPages, uint8_t nPages);

// Container for multiple tile maps and used palette.
typedef struct
{
	const uint16_t** ppPackedTilePages;	// List of packed pages.
	uint8_t  NumTilePages;				// Number of pages.
	const uint16_t* pPaletteData;		// Palette data.
	uint16_t PaletteCountSrc;			// Number of palette entries present in data.
	uint16_t PaletteStartDest;			// First (destination) color of the palette
	uint16_t PaletteCountDest;			// Number of destination colors. If greater than PaletteCountSrc, the colors from data will be repeated.
} TileGraphics;

// Unpacks multiple pages, sets up associated palette, all from a single structure.
void UnpackTileGraphics (uint8_t firstDestPage, const TileGraphics* pSrcData);

#ifdef __cplusplus
} // extern "C"
#endif // __cplusplus

#endif // __TILEUNPACK_H__
