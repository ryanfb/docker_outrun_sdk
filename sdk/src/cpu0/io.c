#include "io.h"

static uint8_t s_ppi_mirror[4] = { 0 };

static void PPI_Write_Flush (PPIRegister idx)
{
	((volatile uint8_t*)(IO_PPI_BASE+idx))[1] = s_ppi_mirror[idx];
}

void PPI_Write_Register (PPIRegister idx, uint8_t value, uint8_t mask)
{
	uint8_t mirrorValue = s_ppi_mirror[idx];
	mirrorValue &= ~mask;
	mirrorValue |= (value & mask);
	s_ppi_mirror[idx] = mirrorValue;
	PPI_Write_Flush (idx);
}

uint8_t PPI_Read_Register (PPIRegister idx)
{
	return ((volatile uint8_t*)(IO_PPI_BASE+idx))[1];
}

void PPI_Init ()
{
	// Init PPI, mode 0 (basic I/O). A=in; B/C=out.
	PPI_Write_Register (PPI_CONTROL_WORD, 0x90, 0xff);

	// Reset sound, blank screen.
	PPI_Write_Register (PPI_WRITE_PORT_C, 0, 0xff);
}

void PPI_EnableScreen (bool enableScreen)
{
	PPI_Write_Register (PPI_WRITE_PORT_C, enableScreen ? PPI_C_NOKILL : 0, PPI_C_NOKILL);
}

void PPI_EnableSound (bool enableSound)
{
	PPI_Write_Register (PPI_WRITE_PORT_C, enableSound ? PPI_C_NOSOUNDRESET : 0, PPI_C_NOSOUNDRESET);
}

void INPUT_TriggerAnalogInputSample (AnalogInputPort analogPort)
{
	// There are 6 inputs on the 20 pin connector ('D') connected to the 74HC4051 analog multiplexer, 3 are used for the game.
	// First off, select the input for the ADC on the multiplexer.
	analogPort &= 7; 
	uint8_t portC = s_ppi_mirror[PPI_WRITE_PORT_C];
	portC &= ~(7 << 2);
	portC |= (analogPort << 2);
	s_ppi_mirror[PPI_WRITE_PORT_C] = portC;
	PPI_Write_Flush (PPI_WRITE_PORT_C);

	// Now start the ADC sample.
	((volatile uint8_t*)IO_ADC_BASE_PTR)[1] = analogPort; // Write anything, really.
}

// Either wait at least 64 cycles @ 6Mhz, or block until the interrupt.
// Also, don't read the same sample twice with the wait flag; the first read resets the interrupt; the second will never complete.
uint8_t INPUT_ReadAnalogInput (bool bWait)
{
	// We could wait for the PPI on port A here. I'm not sure whether this is set up in MAME.
	// if (bWait)
	// 	while (PPI_Read_Register (PPI_READ_PORT_A) & PPI_A_ADIR);
	
	return ((volatile uint8_t*)IO_ADC_BASE_PTR)[1];
}

uint8_t INPUT_ReadDigital (DigitalInputPort portIdx)
{
	return ((volatile uint8_t*)(IO_DIGITAL_INPUT_BASE+portIdx))[1];
}
