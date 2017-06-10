#ifndef __PALETTE_H__
#define __PALETTE_H__

/*
	The palette consists of 8K ram = 4K 16-bit color entries.
	The bottom 12 bits constitute a 4:4:4 BGR color; bits 12-14 add an extra bit of precision.
	Bit 15 is used in highlight mode, where the background is brightened if the bit is set,
	or darkened when the bit is not set.
	Accessing palette ram during the active display period will cause palette noise.

	The first 64 colors are used as 8 sets of 8 for the text layer.
	The road layer uses the following color ranges: 0x400-0x40f, 0x420-0x43f and 0x780-0x7ff.
	Sprites use 128 16-color (4bpp) palettes, starting at 0x800.
*/

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" 
{
#endif // __cplusplus

#define PALETTE_SPRITE_START_IDX 0x800 // 128 * 16 colors.

void PALETTE_SetColorRGBH (uint16_t color, uint8_t red, uint8_t green, uint8_t blue, bool highlight);
void PALETTE_SetColorRGB (uint16_t color, uint8_t red, uint8_t green, uint8_t blue);
void PALETTE_SetColor16 (uint16_t color, uint16_t color16);
uint16_t PALETTE_GetColor16 (uint16_t color);

#ifdef __cplusplus
} // extern "C"
#endif // __cplusplus

#endif // __PALETTE_H__
