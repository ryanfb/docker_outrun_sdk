#include <irq.h>
#include "subcpu.h"
#include "road.h"

extern void cpu1_wait_for_command ();
extern void cpu1_clear_command ();

void main ()
{
	// Init road.
	int i,j;
	short distance = 0;

	// Palette flags: 0x1 = Road texturing shade.
	//                0x2 = Enable lane striping.
	//                0x4 = Disable outer striping.
	//				  0x8 = Dark center striping (pair with 0x1).
	static const unsigned short roadColors[4] = 
	{
		0x0 | (14<<8),
		0x2 | (15<<8) | 0x4 | 0x8 | 0x1,
		0x0 | (14<<8),
		0x2 | (15<<8) | 0x4 | 0x8 | 0x1,
	};

	// Scanline to rom pattern index.
	static short idxtable[ROAD_SCREEN_HEIGHT];

	// Depth table (256 units = 12m)
	static short ztable[ROAD_SCREEN_HEIGHT];

	short dx = -10, ddx = 0, current_x = 0;

	// Set up tables.


	// Road 0 only.
	((unsigned char*)SUB_ROAD_CONTROL)[1] = 0;

	// Set up road twice.
	for (j=0; j<2; j++)
	{
		ddx = 0;
		current_x = 0;

		// Fill the top half with a solid background color.
		for (i=0; i<=112; i++)
			SUB_ROAD_BASE[i] = 0x800 + ((112-i) >> 1);

		// At index 223, the road graphic is 218 pixels wide.
		// For practical applications we can argue that the index equals the number of pixels.
		// Based on our standards, highway lanes are supposed to be 3.50m (US: 3.66m)
		// So we'll say that the three-lane road including some extra space on the side is 12m wide.
		// Lane marks are 3m long, 9m inbetween.
		// If we have a 60 degree FOV, a full-width line (index ~320) should be 10.39 metres away)
		// Our pixels aren't completely square due to the 320x224 resolution, so vertical FOV is different.
		// Vertical FOV should be 46.8264 degrees.
		// Let's use 256 units for 12m (for easy scrolling 9:3). This gives us a visible range of 3km in 16 bits.
		//
		// Our bottom scanline line is at 23.41 degrees.
		// o---------------> view vector.
		// \  
		// |  \
		// |y    \
		// |        \
		// r----------------------- - - -  road surface.
		//
		// At 23.41 degrees, the Z distance is ~2.309 times the camera height (in metres).
		// We'll be 112 scanlines out, which should hit the road at 4m16 (if cam at 1m80).
		// Which translates to an z index value of ~88.68 (with 12m=256 units).
		// Solved for integers: 88.68 = 9932 / 112

		for (i=223; i>=112; i--)
		{
			signed short y = (i-111); // 112..1
			signed short z = 9932 / y; // This should stay under 512.

			// As we hit the road at 4m16, with 60 degrees horizontal vision,
			// tan(30deg) = 0.5*width / 4.16m; width = 9.607m.
			// The actual road is 12m wide, so 1.249 times the full width (320) = 399.7.
			short xscale = (400 * 88) / z;

			signed short idx = xscale;
			ztable[i] = z;
			idxtable[i] = idx;

			SUB_ROAD_BASE[i] = idx;
			SUB_ROAD_BASE[(0xC00/2)+idx] = ((z >> 4) & 0x0f00) | roadColors[(z >> 6) & 0x3];
			
			// Scroll values.

			SUB_ROAD_BASE[(0x400/2)+idx] = 0x5b4 + 160; //  + (((current_x >> 3) * (z >> 3)) >> 11);
			ddx += dx;
			current_x += dx; // ddx;
		}

/*		for (i=0; i<512; i++)
		{
			int offs = i;
			unsigned short color = 0;
			if (offs&16)
				color |= (4 | 256);
			if (offs&32)
				color |= 2;
			SUB_ROAD_BASE[(0xC00/2)+i] = color | ((i>>5)<<8); //  | (i<<8); // i|1|4;
		}
*/
		// Wait (for safety, so we're in the vblank period, not accidentally at scanline 0), then flip, and wait another frame to be sure that the swap has succeeded

		// Safe with vblank wait.
		ROAD_SwapBuffers ();
	}

	// We were just at a vblank, so we're in time for the flag which hits at scanline 0.
	for (;;)
	{
//		wait_vblank ();
		
		// Scanline 224
		distance -= 8; // ~120km/h

		// This apparently takes <38 scanlines.
		for (i=223; i>=112; i--)
		{
			short z = (ztable[i] - distance);
			short idx = idxtable[i];
			SUB_ROAD_BASE[(0xC00/2)+idx] = roadColors[(z >> 6) & 0x3];
		}

		// Scanline 250
		// Same as ROAD_ScheduleSwapBuffers.
		asm("tst.b 0x90001"); // We'll flip at scanline 0.

		IRQ4_Wait ();

/*		// Dit duurt 5 frames? WAT DE HEL.
		for (i=0; i<480; i++)
		{
			int offs = i-(t*8);
			offs &= 0xff;
			unsigned short color = 0;
			if (offs&16)
				color |= 4;
			if (offs&64)
				color |= (2 | (14 << 8) | 1|8);
			else 
				color |= (15 << 8);
			SUB_ROAD_BASE[(0xC00/2)+i] = color; // | ((i>>5)<<8); //  | (i<<8); // i|1|4;
		}

		// 0x654 looks like the correct center in mame; so the ROM isn't completely centered or mame is wrong.
		for (i=0; i<480; i++)
			SUB_ROAD_BASE[0x200+i] = 0x5b4 + 160;

//		for (i=0; i<224; i++)
//			SUB_ROAD_BASE[i] = i+t; // (i << 1);
*/
		asm("nop; nop; nop");
	}

	for (;;)
	{
		cpu1_wait_for_command ();
//		i = ((volatile unsigned char*)SUB_ROAD_CONTROL)[1]; // Flip. Dit flipt iig iets.
		cpu1_clear_command ();
	}
}
