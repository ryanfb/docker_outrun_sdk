#ifndef __ORSOUND_H__
#define __ORSOUND_H__

#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus

#include <stdint.h>

/*
	Sound command sequences are sent to register 3 (0xffffff07) on the memory mapper,
	which routes it to the Z80 and generates an interrupt.

	With the original programming of the Z80, continuously sequences of 8 bytes are sent,
	the first being a command byte, and the following seven are 'global' registers,
	continuously being written with mostly the same sequence.

	This requires rather critical timing - it needs to be kept alive, fast enough for the Z80
	not to time out, but slow enough for it to handle its interrupts at the lower 4MHz clock.

	The sound routine is run on IRQ2 of CPU0, which asserts 3 times per frame (180Hz).
	In the main program, a 32 byte ring buffer is filled with commands, and the IRQ routine
	drains this buffer one command at a time, followed by the 7 global registers.
	Effectively 22.5 complete commands per second.

	When the ring buffer has drained, the dummy command 0x80 is sent.
*/

// Commands found by experminenting - these start the various songs and sound effects:
enum
{
	ORSoundCmd_PassingBreeze 		= 0x81,	// music
	ORSoundCmd_SplashWave    		= 0x82, // music
	ORSoundCmd_InsertCoin			= 0x84,
	ORSoundCmd_MagicalSoundShower 	= 0x85, // music
	ORSoundCmd_ExtendedPlay			= 0x86, // checkpoint (beep)
	ORSoundCmd_BurnOut				= 0x8a,
	ORSoundCmd_Applause				= 0x8d,
	ORSoundCmd_Crash1				= 0x8f, 
	ORSoundCmd_Crash2				= 0x90,
	ORSoundCmd_StartLight			= 0x94,
	ORSoundCmd_StartLightGreen		= 0x95,
	ORSoundCmd_Checkpoint			= 0x9d, // voice
	ORSoundCmd_Congratulations		= 0x9e, // voice
	ORSoundCmd_GetReady				= 0x9f, // voice
	ORSoundCmd_Skidding				= 0xa0,
	ORSoundCmd_Waves				= 0xa4,
	ORSoundCmd_LastWave 			= 0xa5,	// music
};

// Enables sound playback.
void orsound_enable (uint8_t enable);

// Sets one of the 7 sound variables. reg is 1-based (0 contains the command, but isn't copied from the registers).
void orsound_write_register (uint8_t reg, uint8_t value);

// Writes a new command to the sound buffer.
void orsound_write_command (uint8_t command);

// Init routine (used to be called from startup code).
void orsound_init (void);

// Update routine (to be called from IRQ 2).
void orsound_update (void);

#ifdef __cplusplus
} // __extern "C"
#endif // __cplusplus

#endif // __ORSOUND_H__
