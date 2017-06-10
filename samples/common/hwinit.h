#ifndef __HWINIT_H__
#define __HWINIT_H__

#ifndef CPU0
#error "This file deals specifically with the subsystems accessible to CPU 0 and cannot be used from CPU 1!"
#endif

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#include <stdint.h>

// Flags.
enum
{
	HWINIT_Clear_Text = 1,					// Clears TEXT ram.
	HWINIT_Reset_Sprites = 2,				// Clears the sprite list and flips it.
	HWINIT_Reset_PPI = 4,					// Initializes the PPI in/output directions.
	HWINIT_Reset_Tiles = 8,
	HWINIT_Reset_Road_Palette = 8,			// Clears the ROAD section of the palette to a single color (so we won't see anything happening regardless of what CPU1 is doing)

	HWINIT_EnableScreen = 0x4000,
	HWINIT_EnableSound 	= 0x8000,

	HWINIT_All = 0xffff,
	HWINIT_Default = (HWINIT_All & ~HWINIT_EnableSound),
};

// Performs initialization of Out Run hardware subsystems into a clean state.
// This is provided for ease of use in samples. Normally you'd choose what to do with each subsystem upon initialization.
void HW_Init (uint16_t flags, uint16_t bgcolor);

#ifdef __cplusplus
} // extern "C"
#endif // __cplusplus

#endif // __HWINIT_H__
