#include <maincpu.h>
#include <stdint.h>
#include <stdbool.h>

void PALETTE_SetColorRGBH (uint16_t color, uint8_t red, uint8_t green, uint8_t blue, bool highlight)
{
	uint16_t color16 = 
		// Low bits go to bit 12-14
		((((uint16_t)red  )&1) << 12) |
		((((uint16_t)green)&1) << 13) |
		((((uint16_t)blue )&1) << 14) |
		// High 4 bits are stored 4:4:4
		((((uint16_t)red  )&0x1e) >> 1) |
		((((uint16_t)green)&0x1e) << 3) | 
		((((uint16_t)blue )&0x1e) << 7) |
		// Highlight bit.
		(highlight ? 0x8000 : 0);
		
	PALETTE_RAM_BASE[color & 0xfff] = color16;
}

void PALETTE_SetColorRGB (uint16_t color, uint8_t red, uint8_t green, uint8_t blue)
{
	PALETTE_SetColorRGBH (color, red, green, blue, 0);
}

void PALETTE_SetColor16 (uint16_t color, uint16_t color16)
{
	PALETTE_RAM_BASE[color & 0xfff] = color16;
}

uint16_t PALETTE_GetColor16 (uint16_t color)
{
	return PALETTE_RAM_BASE[color & 0xfff];
}
