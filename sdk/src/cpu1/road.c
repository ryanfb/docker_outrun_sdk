#include <irq.h>
#include "road.h"

#ifdef CPU0

#include "palette.h"

void ROAD_ResetPalette (unsigned short bgColor)
{
	unsigned short i;
	for (i=0x400; i<0x410; i++)
		PALETTE_SetColor16 (i, bgColor);
	for (i=0x420; i<0x440; i++)
		PALETTE_SetColor16 (i, bgColor);

	// Background colors (scanline fill mode).
	for (i=0; i<128; i++)
		PALETTE_SetColor16 (i+0x780, bgColor);
}

#endif // CPU0

#ifdef CPU1

// Sets road priorities. From Mame's segaic16.c:
//   0 = road 0 only visible
//   1 = both roads visible, road 0 has priority
//   2 = both roads visible, road 1 has priority
//   3 = road 1 only visible
void ROAD_SetPriority (unsigned char priority)
{
	priority &= 3;
	((uint8_t*)SUB_ROAD_CONTROL)[1] = priority;
}

// Schedules a buffer swap, which will occur on scanline 0. This will swap the RAM
// that is drawn by the road hardware with the RAM that is being written to at SUB_ROAD_BASE.
// Somewhat careful timing is needed, which Mame doesn't emulate properly.
// Since we only have an interrupt on the VBLANK at scanline 223, we should schedule the flip
// and finish writing the RAM contents before line 262/0, or delay everything until that line.
void ROAD_ScheduleSwapBuffers ()
{
	asm("tst.b 0x90001");
}

// Safe swap for initialization code. Could take two frames but is safe to call at any time.
void ROAD_SwapBuffers ()
{
	// Wait for scanline 224 and schedule a flip at scanline 0.
	IRQ4_Wait ();
	ROAD_ScheduleSwapBuffers ();

	// Wait again for scanline 224.
	IRQ4_Wait ();
}

// Reset to a single background color (0x7f, palette entry 0x7ff).
void ROAD_Reset ()
{
	unsigned short i,y;

	// Road 0 only.
	ROAD_SetPriority (0);

	// Reset the pattern and scroll registers.
	IRQ4_Wait ();
	for (y=0; y<ROAD_SCREEN_HEIGHT; y++)
	{
		ROAD_PATTERN_0[y] = ROAD_SOLID_COLOR(0x7f);
		ROAD_SCROLL_0[y]  = ROAD_SCROLL_CENTER;
	}

	// And flip. Assumes we can do the above between scanline 224 and 262 (=0).
	ROAD_ScheduleSwapBuffers ();
}

#endif // CPU1
