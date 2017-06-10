#include <io.h>
#include "text.h"
#include "hwinit.h"
#include "palette.h"
#include <tile.h>

static void ROAD_BlankPalette (uint16_t color)
{
	for (uint16_t i=0x400; i<0x410; i++)
		PALETTE_SetColor16 (i, color);
	
	for (uint16_t i=0x420; i<0x440; i++)
		PALETTE_SetColor16 (i, color);

	// Background colors (scanline fill mode).
	for (uint16_t i=0; i<128; i++)
		PALETTE_SetColor16 (i+0x780, color);
}

void HW_Init (uint16_t flags, uint16_t bgcolor)
{
	// Initialize PPI in/output ports. Should be done before writing to them.
	// This mutes sound and disables the screen.
	if (flags & HWINIT_Reset_PPI)
	{
		PPI_Init ();
	}
	
	// Clear text ram to spaces.
	if (flags & HWINIT_Clear_Text)
	{
		TEXT_ClearTextRAM ();
		WATCHDOG_Reset ();
	}
	
	// Reset a single tile ram page, and switch to it.
	if (flags & HWINIT_Reset_Tiles)
	{
		TILE_Reset ();
		WATCHDOG_Reset ();
	}
	
	if (flags & HWINIT_Reset_Sprites)
	{
		// Hacky version. Will clean up later.
		volatile uint16_t * const __spriteRam  = (volatile uint16_t*)0x130000;

		// Dit zou alle sprites wel zo uit moeten zetten zegmaar.
		for (uint8_t i=0; i<128; i++)
		{
			volatile uint32_t* pSprite = ((volatile uint32_t*)__spriteRam)+(i*4);
			pSprite[0] = 0xc0000000;
			pSprite[1] = 0;
			pSprite[2] = 0;
			pSprite[3] = 0;
		}

		// Anyway:
		*((volatile uint8_t*)0x140070) = 0xff; // Flip sprite bank. Shouldn't this be 71?
	
		WATCHDOG_Reset ();
	}
	
	// Clear road palette
	if (flags & HWINIT_Reset_Road_Palette)
	{
		ROAD_BlankPalette (bgcolor);
		WATCHDOG_Reset ();
	}

	// If we want a working screen, enable it.	
	if (flags & HWINIT_EnableScreen)
		PPI_EnableScreen (true);
	
	// Sound too (actually, there's still the AMP mute on the output pins)
	if (flags & HWINIT_EnableSound)
		PPI_EnableSound (true);
}
