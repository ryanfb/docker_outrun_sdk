#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include "palette.h"
#include "text.h"
#include "io.h"
#include "hwinit.h"

//=====================================================================================================================================================
// Input test.
//=====================================================================================================================================================

typedef struct
{
	const char* desc;
	const char* descOn, *descOff;
} INPUT_BIT_DESCRIPTION;

typedef struct
{
	DigitalInputPort port;
	const char* description;
	TEXTCOLOR numColor;
	uint8_t x, y, width, height;
	INPUT_BIT_DESCRIPTION bitNames[8];
} DIGITAL_INPUT;

#define TOP_LEFT 3

// Displayed slightly out of order so the dips appear at the top.
const DIGITAL_INPUT digitalInputs[] =
{
	{
		DIGITAL_DIPSW_A,
		"Dip Switch A",
		TEXT_Red,
		TEXT_SCREEN_VISIBLE_XSTART+TOP_LEFT, 1, 18, 14,
		{
			{ "\007Coin A" },
			{ "\007Coin A" },
			{ "\007Coin A" },
			{ "\007Coin A" },
			{ "\007Coin B" },
			{ "\007Coin B" },
			{ "\007Coin B" },
			{ "\007Coin B" },
		}
	},
	{
		DIGITAL_DIPSW_B,
		"Dip Switch B",
		TEXT_Red,
		TEXT_SCREEN_VISIBLE_XSTART+20, 1, 18, 14,
		{
			{ "\007Cabinet Type" },
			{ "\007Cabinet Type" },
			{ "Demo Sounds" },
			{ NULL }, // 3
			{ "\007Time Adj." },
			{ "\007Time Adj." },
			{ "\007Difficulty" },
			{ "\007Difficulty" },
		}
	},
	{
		DIGITAL_INPUT_1,
		"Digital Input 1",
		TEXT_Blue,
		TEXT_SCREEN_VISIBLE_XSTART+TOP_LEFT, 11, 18, 14,
		{
			{ "\007A8 CN.G 50p" },
			{ "Test   " },
			{ "Service" },
			{ "Start" }, // 3
			{ "Gear ", "Low Gear ", "High Gear" }, // 4
			{ "Turbo" },
			{ "Coin1" }, // 6
			{ "Coin2" }, // 7
		}
	},
	{
		DIGITAL_INPUT_2,
		"Digital Input 2",
		TEXT_Blue,
		TEXT_SCREEN_VISIBLE_XSTART+20, 11, 18, 14,
		{
			{ "\007A16 CN.G 50p" },
			{ "\007A15" },
			{ "\007A14" },
			{ "\007A13" }, // 3
			{ "\007A12" }, // 4
			{ "\007A11" },
			{ "\007A10" }, // 6
			{ "\007A9"  }, // 7
		}
	},
};

// These are in the same order as the ports.
const char* analogInputs[] = 
{
	"Steering",
	"Accelerator",
	"Brake",
	"\007A5 CN.D 20P",
	"\007B8",
	"\007A8",
};

void PrintDigitalInput (uint8_t dips, const INPUT_BIT_DESCRIPTION* pDescriptions, const char* title, TEXTCOLOR numColor, uint8_t displayMask)
{
	if (title && displayMask == 0xff)
	{
		TEXT_SetColor (TEXT_White);
		TEXT_Write (title);
	}
	TEXT_Write ("\n");
	
	if (displayMask != 0xff)
		numColor = TEXT_Green; // Little highlight for when toggled.

	uint8_t y;
	uint8_t mask = 1;
	for (y=0; y<8; y++)
	{
		if (displayMask & mask)
		{
			// Prefix with bit number
			TEXT_SetColor (numColor);
			TEXT_WriteChar (y+'0');
			TEXT_Write (" ");
			
			// Draw switch graphic ('on' or 'off')
			// 9 = on, 11 = off.
			bool dipOn = !(dips & mask); // Inverted!
			TEXT_SetColor (dipOn ? TEXT_White : TEXT_Gray);
			TEXTGLYPH gl = dipOn ? 9 : 11;
			TEXT_WriteRawChar (gl++);
			TEXT_WriteRawChar (gl++);

			// Description. Can be state specific (low gear -> high gear)
			TEXT_Write (" ");
			if (!dipOn) // keep it white otherwise
				TEXT_SetColor (TEXT_Yellow);
			
			const char* desc = "";
			const INPUT_BIT_DESCRIPTION* pDesc = &pDescriptions[y];
			if (pDesc->desc)
				desc = pDesc->desc;
			if (dipOn && pDesc->descOn)
				desc = pDesc->descOn;
			else if (!dipOn && pDesc->descOff)
				desc = pDesc->descOff;
			TEXT_Write (desc);
		}

		TEXT_Write ("\n");
		mask <<= 1;
	}
}

void PrintAllInputs (bool bFirstTime)
{
	// First time we don't mask out anything.
	static uint8_t lastDigitalValues[4];
	static uint8_t lastAnalogValues[6];

	// Digital inputs / dip switches.
	int i;
	const size_t nDigitalInputs = sizeof(digitalInputs)/sizeof(DIGITAL_INPUT);
	for (i=0; i<nDigitalInputs; i++)
	{
		const DIGITAL_INPUT* inputDesc = &digitalInputs[i];
		uint8_t dips = INPUT_ReadDigital(inputDesc->port);
		TEXT_SetWindow (inputDesc->x, inputDesc->y, inputDesc->x+inputDesc->width, inputDesc->y+inputDesc->height, false);
		
		uint8_t mask = 0xff;
		if (!bFirstTime)
			mask = lastDigitalValues[i] ^ dips;
		PrintDigitalInput (dips, inputDesc->bitNames, inputDesc->description, inputDesc->numColor, mask);
		
		lastDigitalValues[i] = dips;
	}
	
	// Analog inputs.
	TEXT_SetWindow (TEXT_SCREEN_VISIBLE_XSTART+TOP_LEFT, 21, 64, 28, false);
	TEXT_SetColor (TEXT_White);
	TEXT_Write (bFirstTime ? "Analog Inputs\n" : "\n");

	uint8_t analogInput;
	for (analogInput = 0; analogInput < 5; analogInput++) // 5 is all that'll fit on the screen.
	{
		// Read an analog input, then.
		// Todo: verify that this works on real hardware (timing sensitive; unless the wait works).
		INPUT_TriggerAnalogInputSample (analogInput);
		uint8_t analogValue = INPUT_ReadAnalogInput (true);
		
		if (bFirstTime || analogValue != lastAnalogValues[analogInput])
		{
			TEXT_SetColor (bFirstTime ? TEXT_Cyan : TEXT_Green); // Green when changed.
			TEXT_WriteChar ('0'+analogInput);
			TEXT_WriteChar (' ');

			TEXT_SetColor (TEXT_White);
			TEXT_WriteHex8 (analogValue);
			
			const char* desc = analogInputs[analogInput];
			if (bFirstTime && desc)
			{
				TEXT_WriteChar(' ');
				TEXT_SetColor (TEXT_Yellow);
				TEXT_Write (desc);
			}
			
			// Draw bar.
			TEXT_SetColor (analogInput == 0 ? TEXT_Cyan : TEXT_Green);
			TEXT_GotoXY (17, analogInput+1);
			uint16_t roundUpValue = analogValue + 8;
			uint8_t barLength = roundUpValue >> 4;
			for (i=0; i<barLength-1; i++)
			{
				if (i == 8 && analogInput != 0)
					TEXT_SetColor (TEXT_Yellow);
				else if (i == 12 && analogInput != 0)
					TEXT_SetColor (TEXT_Red);
					
				TEXT_WriteRawChar(0x1fd);
			}
			if (barLength)
				TEXT_WriteRawChar((roundUpValue & 8) ? 0x1fd : 0x1fe);
			for (i=barLength; i<16; i++)
				TEXT_Write(" ");
		}			
		
		lastAnalogValues[analogInput] = analogValue;
		TEXT_Write("\n");
	}
}

//-----------------------------------------------------------------------------------------------------------------------------------------------------

int main ()
{
	// Init PPI, blank screen, mute sound.
	HW_Init (HWINIT_Default, 0x321);

	// Setup basic colors for text.
	TEXT_InitDefaultPalette();
	
	// 'Safe' area as emperically determined on my 14" Sony Wega CRT TV.
	TEXT_DrawRect (TEXT_SCREEN_VISIBLE_XSTART+2, 0, TEXT_SCREEN_WIDTH-1, TEXT_SCREEN_HEIGHT, 0xffff);
	
	for (;;)
	{
		// First time we don't mask out anything.
		static bool bFirstTime = true;
		PrintAllInputs (bFirstTime);
		bFirstTime = false;
	}
}
