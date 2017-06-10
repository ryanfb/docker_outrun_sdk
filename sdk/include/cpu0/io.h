#ifndef __IO_H__
#define __IO_H__

#include "maincpu.h"

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" 
{
#endif // __cplusplus

// PPI8255 at 0x140000
#define IO_PPI_BASE IO_REGS_BASE

/*
PPI 8255 in Outrun (Video board).
PORT A: Motor limit SW input (Bit 0..5); !ADIR (ADC control)=Bit 6
PORT B: Motor board (output)
PORT C:
bit 0: !PRS  (Sound Reset - Resets the Z80)
    1: CONT
    2: ADC0
    3: ADC1
    4: ADC2
    5: !KILL (Screen Blank)
    6: SG0   (Sprite HW)
    7: SG1
*/

// PPI Registers.
typedef enum __attribute__ ((__packed__))
{
	PPI_READ_PORT_A  = 0,
	PPI_WRITE_PORT_B = 1,
	PPI_WRITE_PORT_C = 2,
	PPI_CONTROL_WORD = 3,
} PPIRegister;

// PPI Port bits.
// Port A (input)
enum __attribute__ ((__packed__))
{
	PPI_A_ADIR = 0x40, // Interrupt from the ADC when done.
};

// Port C (output)
enum __attribute__ ((__packed__))
{
	PPI_C_NOSOUNDRESET = 0x1,
	PPI_C_CONT         = 0x2,
	PPI_C_ADC_0        = 0x4,	// ADC input select (3 bits).
	PPI_C_ADC_1        = 0x8,
	PPI_C_ADC_2        = 0x10,
	PPI_C_NOKILL       = 0x20, 	// Screen blank when set.
	PPI_C_SG0          = 0x40,
	PPI_C_SG1          = 0x80,
};

// Shadowed and then written, so we can set invididual bits.
// This is needed if we don't want to remember the previous setting everywhere; we can't read back the value previously set otherwise.
void PPI_Write_Register (PPIRegister ppiRegister, uint8_t value, uint8_t mask);
uint8_t PPI_Read_Register (PPIRegister ppiRegister);

// Init PPI, mode 0 (basic I/O). A=in; B/C=out.
void PPI_Init ();

// Helper functions.
void PPI_EnableScreen (bool enableScreen);
void PPI_EnableSound (bool enableSound);

//---------------------------------------------------------------------------------------------------------------------------------------------------------------------

// Digital Inputs (0x140010)
// 4 digital input ports (2x8 bit input, 2x8 bit dip switches).
#define IO_DIGITAL_INPUT_BASE (IO_REGS_BASE+0x10/2)

// Digital input ports, active low.
typedef enum __attribute__ ((__packed__))
{
	DIGITAL_INPUT_1 = 0x0,
	DIGITAL_INPUT_2 = 0x1,
	DIGITAL_DIPSW_A = 0x2, // Used for coins needed.
	DIGITAL_DIPSW_B = 0x3, // Difficulty settings, cabinet type.
} DigitalInputPort;

uint8_t INPUT_ReadDigital (DigitalInputPort digitalPort);

// Digital 'user' inputs. Physical buttons on the cabinet.
// These can be read from DIGITAL_INPUT_1
// Note that the bits are inverted, pushing a button shorts it to ground (0), otherwise the bit is high.
enum __attribute__ ((__packed__))
{
	DIGITAL_INPUT_1_Test 	= 1 << 1,
	DIGITAL_INPUT_1_Service = 1 << 2,
	DIGITAL_INPUT_1_Start 	= 1 << 3,
	DIGITAL_INPUT_1_Gear 	= 1 << 4,
	DIGITAL_INPUT_1_Turbo 	= 1 << 5,
	DIGITAL_INPUT_1_Coin1 	= 1 << 6,
	DIGITAL_INPUT_1_Coin2 	= 1 << 7,
};

//---------------------------------------------------------------------------------------------------------------------------------------------------------------------

// Digital Output (0x140020)
#define IO_DIGITAL_OUT_ADDR 0x140020

// Digital output port (8 bits) on a ULN2003 Darlington transistor array.
// Outputs are pulled to ground when 'on'.
// We can use pins 17 through 23 when ignoring the external amplifier and shaker motor.
enum __attribute__ ((__packed__))
{
	DIGITAL_OUT_MUTE_EXTERNAL 	= 0x01, // Pin A23 connector G
	DIGITAL_OUT_BRAKE_LAMP    	= 0x02, // Pin A22 connector G
	DIGITAL_OUT_START_LAMP    	= 0x04, // Pin A21 connector G
	DIGITAL_OUT_COIN_METER_2  	= 0x08, // Pin A20 connector G
	DIGITAL_OUT_COIN_METER_1  	= 0x10, // Pin A19 connector G
	DIGITAL_OUT_UPRIGHT_SHAKER 	= 0x20, // Pin A18 connector G
	DIGITAL_OUT_MUTE_INTERNAL  	= 0x80,
};

static inline void IO_WriteDigital (uint8_t digitalOut) { *((volatile uint8_t*)(IO_DIGITAL_OUT_ADDR+1)) = digitalOut; }

//---------------------------------------------------------------------------------------------------------------------------------------------------------------------

// ADC at 0x140030
#define IO_ADC_BASE_ADDR 0x140030
#define IO_ADC_BASE_PTR ((volatile uint16_t*)(IO_ADC_BASE_ADDR))

// Analog input helpers.
// Selects the input for the ADC (IC116) by way of setting the selector lines on the 74HC4051 multiplexer (IC117).
// These are connected to bits 2..4 of port C on the PPI.
// Sending a write+chip select to the ADC will start a sample conversion. Once it's done it'll assert the interrupt which is connected to the PPI on port A ("ADIR")
typedef enum __attribute__ ((__packed__))
{
	AnalogInput0_Steering		= 0,
	AnalogInput1_Accelerator	= 1,
	AnalogInput2_Brake			= 2,
} AnalogInputPort;

// Sets up the input and triggers a read (sample).
void INPUT_TriggerAnalogInputSample (AnalogInputPort port);

// Read the value from the ADC. Make sure to wait long enough, or check the ADIR status.
// If a sample wasn't triggered, this could hang (when waiting).
uint8_t INPUT_ReadAnalogInput (bool bWait);

//---------------------------------------------------------------------------------------------------------------------------------------------------------------------

// Watchdog (0x140060)
#define WATCHDOG_ADDR 0x140060
static inline void WATCHDOG_Reset () { __asm__("tst.w 0x140060.l"); }

//---------------------------------------------------------------------------------------------------------------------------------------------------------------------

// Sprites (0x140070)

#ifdef __cplusplus
} // extern "C"
#endif // __cplusplus

#endif // __IO_H__
