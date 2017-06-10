#include "hwinit.h"
#include "text.h"
#include "tile.h"
#include "palette.h"
#include "input.h"
#include "menu.h"
#include <irq.h>
#include <stdint.h>
#include <io.h>
#include "orsound.h"
#include "tileunpack.h"
#include <sprite.h>

extern const TileGraphics RadioGraphics;

void SetupGradient ()
{
	const uint8_t colorStart[3] = { 0x00, 0x40, 0x80 };
	const uint8_t colorEnd[3]   = { 0x40, 0xe0, 0xf0 };
	
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

// This may not be emulated correclty? Or we can't read from sprite ram.
void SPRITE_SetupStatic (volatile SpriteData* pDest, int16_t left, int16_t top, uint8_t bank, uint16_t offset, int8_t pitch, uint8_t height, uint8_t palette)
{
	pDest->Hidden = pDest->Hidden2 = pDest->EndOfList = 0;
	pDest->EndOfList = 0;
	pDest->Bank = (bank & 3);
	pDest->BankOffset = offset;
	pDest->Top = (top + SPRITE_ORIGIN_TOP);
	pDest->Left = (left + SPRITE_ORIGIN_LEFT);
	pDest->Pitch = pitch;
	pDest->EnableShadows = 0;
	pDest->Priority = 3;
	pDest->HorizontalZoomFactor = pDest->VerticalZoomFactor = 0x200;
	pDest->DrawTopToBottom = pDest->DrawLeftToRight = 1;
	pDest->FlipHorizontal = 1;
	pDest->Height = height - 1;
	pDest->PaletteIdx = palette;
}

void SetupRadioSprites ()
{
	// Sprite palettes.
	static const uint16_t RadioSpritePalette[] = 
	{
		// Radio.
		0x000, 0xfff, 0x0f0, 0x00f, 0x000, 0x888, 0x999, 0xaaa, 0xbbb, 0xccc, 0xddd, 0x0f0, 0x0f0, 0x00f, 0x0f0, 0x000,
		0x000, 0xfff, 0x000, 0x888, 0x999, 0xaaa, 0xbbb, 0xccc, 0xddd, 0xeee, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, // Knob
	};
	
	for (uint16_t i=0; i<sizeof(RadioSpritePalette)/sizeof(uint16_t); i++)
		PALETTE_SetColor16 (PALETTE_SPRITE_START_IDX+i, RadioSpritePalette[i]);

	// This doesn't work too well in mame. Weird.
	IRQ4_Wait ();
	IRQ4_Wait ();
	volatile SpriteData* pSprite = SpriteList;
	SPRITE_SetupStatic (pSprite++, 122, 166, 3, 0x42c5, 17, 30, 0);
	// 07bb 42b1 0954  -> y=187
	SPRITE_SetupStatic (pSprite++, 214, 169, 3, 0x424d,  4, 25, 1);

	// Terminate list.	
	pSprite->Hidden = pSprite->Hidden2 = pSprite->EndOfList = 1;
	
	IRQ4_Wait ();
	IRQ4_Wait ();
	SPRITE_SwapBuffers (); // Hoe dan.
	IRQ4_Wait ();
	IRQ4_Wait ();
}

void SetupBackgroundGraphics ()
{
	// Palette used for text (title, menu, hint).
	TEXT_InitDefaultPalette ();

	// Palette gradient (drawn by the road hardware)
	SetupGradient ();
	
	// Setup background graphic.
	UnpackTileGraphics (1, &RadioGraphics);

	// Enable row scroll, for the scrolling clouds, and duplicate the page horizontally.
	TileRegisters.BackgroundMapPages.Pages = 0x0011;
	TileRegisters.BackgroundScrollX.RowScrollEnable = 1; 
	
	// Offset the row scroll values so that the picture aligns with the screen (the background picture is in the bottom left corner in this case).
	for (uint8_t y=0; y<TILE_PAGE_VISIBLE_ROWS; y++)
		TileRegisters.BackgroundRows[y].HorizontalScroll = 24 * 8;
	
	// Radio sprites.
	SetupRadioSprites ();

	// Title text.	
	TEXT_GotoXY (14,1);
	TEXT_SetColor (TEXT_Yellow);
	TEXT_Write ("audio sample");
	
	// Instructions. Left/right to select, Accelerate to play.
	TEXT_GotoXY (7,26);
	TEXT_SetColor (TEXT_White);
	TEXT_WriteRawChar (TEXTGLYPH_ARROWLEFT);
	TEXT_Write (" \006or\010 ");
	TEXT_WriteRawChar (TEXTGLYPH_ARROWRIGHT);
	TEXT_Write ("\006 to choose  \010");
	TEXT_WriteRawChar (TEXTGLYPH_ARROWUP);
	TEXT_Write ("\006 to play");
}

void DrawMute (uint8_t idx, bool muted)
{
	TEXT_GotoXY (25, 21+(idx << 1));
	TEXT_SetColor (muted ? TEXT_White : TEXT_Gray);
	TEXTGLYPH gl = muted ? 9 : 11;
	TEXT_WriteRawChar (gl++);
	TEXT_WriteRawChar (gl++);
}

static uint8_t outputBits = DIGITAL_OUT_MUTE_EXTERNAL|DIGITAL_OUT_MUTE_INTERNAL; // Default, inverted (high = mute off).
bool MuteBit (InputButton button, uint16_t mask)
{
	outputBits ^= (uint8_t)mask;
	IO_WriteDigital (outputBits);
	DrawMute (mask == DIGITAL_OUT_MUTE_INTERNAL ? 0 : 1, !(outputBits & mask));
	return false;
}

void InitializeSound ()
{
	// Let go of the Z80 reset.
	PPI_EnableSound (true);

	// Initialize our service routine and set it up at irq 2.
	orsound_init ();
	IRQ2_SetHandler (orsound_update);
	orsound_enable(1);
	
	// Output (disable mute).
	// Bit set = mute disabled = audio enabled.
	IO_WriteDigital (DIGITAL_OUT_MUTE_EXTERNAL|DIGITAL_OUT_MUTE_INTERNAL);
}

bool PlaySound (InputButton button, uint16_t index)
{
	orsound_write_command(0x93); // NEW_COMMAND
	orsound_write_command(0xA2); // YM_SET_LEVELS.
	orsound_write_command(index); // Waves.
	return true;
}

static const MenuItem s_menuItems[] = 
{
	{ "Magical Sound Shower", PlaySound, ORSoundCmd_MagicalSoundShower },
	{ "Passing Breeze", 	  PlaySound, ORSoundCmd_PassingBreeze },
	{ "Splash Wave", 		  PlaySound, ORSoundCmd_SplashWave },
	{ "Last Wave", 			  PlaySound, ORSoundCmd_LastWave },
	{ "Ocean Waves", 		  PlaySound, ORSoundCmd_Waves },
	{ "\"Get Ready\"", 		  PlaySound, ORSoundCmd_GetReady },
	{ "\"Congratulation\"",   PlaySound, ORSoundCmd_Congratulations },
	{ "\"Check Poi\"", 		  PlaySound, ORSoundCmd_Checkpoint },
	{ "Applause" , 			  PlaySound, ORSoundCmd_Applause },
	{ "Internal Mute", 		  MuteBit,   DIGITAL_OUT_MUTE_INTERNAL },
	{ "External Mute", 		  MuteBit,   DIGITAL_OUT_MUTE_EXTERNAL },
};

// Entry point.
void main ()
{
	// Clean init.
	HW_Init (HWINIT_Default, 0x4f90);

	// Initialize sound hardware and service interrupt, does not play anything yet.
	InitializeSound ();
	
	// Set up background graphics and title/hints.	
	SetupBackgroundGraphics ();
	
	// Initialize input helper.
	InputState inputState;
	INPUT_Init (&inputState);

	// Initialize menu, this draws the initial menu state.
	MenuState menuState;
	MENU_Init (&menuState, s_menuItems, sizeof(s_menuItems)/sizeof(MenuItem), 8, 3, 4);
	
	// Draw 'off' for both mute options.
	DrawMute (0, false);
	DrawMute (1, false);

	// Start 'waves' sound initially.
	PlaySound (Button_Start, 0xa4);
	
	for (;;)
	{
		// Wait for a new vertical blank.
  		IRQ4_Wait();

		// Poll input.
		INPUT_Update (&inputState);
		MENU_Update (&menuState, &inputState);

		// Scroll clouds in the background.		
		static uint16_t cloudScroll = 0;
		++cloudScroll;
		TileRegisters.BackgroundRows[14].HorizontalScroll = (cloudScroll >> 4);
	}
}
