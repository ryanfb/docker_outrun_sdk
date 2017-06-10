#include "hwinit.h"
#include "text.h"
#include "tile.h"
#include "palette.h"
#include <irq.h>
#include <stdint.h>
#include "tileunpack.h"

extern const TileGraphics CloudGraphics;
extern const TileGraphics ShoreGraphics;

void SetupGradient ()
{
	const uint8_t colorStart[3] = { 113,48,243 };
	const uint8_t colorEnd[3] = { 243,178,186 }; // From le website.
	
	uint16_t red   = (uint16_t)colorStart[0] << 8;
	uint16_t green = (uint16_t)colorStart[1] << 8;
	uint16_t blue  = (uint16_t)colorStart[2] << 8;
	
	int16_t d_red   = (colorEnd[0] - colorStart[0]) << 1;
	int16_t d_green = (colorEnd[1] - colorStart[1]) << 1;
	int16_t d_blue  = (colorEnd[2] - colorStart[2]) << 1;
	
	int16_t err_red = 0, err_green = 0, err_blue = 0;
	
	for (uint8_t i=0; i<128; i++)
	{
		int16_t newRed = red + err_red;
		int16_t newGreen = green + err_green;
		int16_t newBlue = blue + err_blue;
		
		err_red = newRed & 0x7ff;
		err_green = newGreen & 0x7ff;
		err_blue = (newBlue & 0x7ff);
		
		PALETTE_SetColorRGB (0x780+i, newRed >> 11, newGreen >> 11, newBlue >> 11);
		
		red += d_red;
		green += d_green;
		blue += d_blue;
	}
}

static void PrintScrollValue (uint16_t val)
{
	char digits[6] = { "     " };
	uint8_t pos = sizeof(digits)-2;
	while (val)
	{
		uint8_t digit = val % 10;
		val /= 10;
		if (digit || val)
		{
			digits[pos] = '0'+digit; pos--;
		}
	}
	TEXT_Write (digits);
}

void main ()
{
	HW_Init (HWINIT_Default, 0xf63);
	TEXT_InitDefaultPalette ();
	SetupGradient ();

	TEXT_GotoXY (13,1);
	TEXT_SetColor (TEXT_Yellow);
	TEXT_Write ("tile map sample");

	UnpackTileGraphics (8, &CloudGraphics);
	UnpackTileGraphics (4, &ShoreGraphics);

	// Set Y offsets.
	TileRegisters.BackgroundMapPages.Pages = 0x009a;
	TileRegisters.BackgroundScrollY.Y = 4*8;

	TileRegisters.ForegroundMapPages.Pages = 0x0047;
	TileRegisters.ForegroundScrollY.Y = 4*8;
	
	// Wait.	
	uint16_t x = 0;
	for (;;)
	{
		// Update scroll position.
		++x;
		TileRegisters.BackgroundScrollX.X = (x >> 1);
		TileRegisters.ForegroundScrollX.X++;
		
		// Display stats.
		TEXT_GotoXY (2,3);
		TEXT_Write ("\010Back\006");
		PrintScrollValue (TileRegisters.BackgroundScrollX.X);
		TEXT_GotoXY (2,4);
		TEXT_Write ("\010Fore\006"); // Their most accomplished album.
		PrintScrollValue (TileRegisters.ForegroundScrollX.X);
		
		IRQ4_Wait ();
	}
}
