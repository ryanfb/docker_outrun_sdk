#include <Platform.h>
#include "lpt.h"
#include "inpout32.h"
#pragma comment(lib, "inpout32.lib")

// Register offsets.
enum
{
	REG_DATA    = 0,
	REG_STATUS  = 1,
	REG_CONTROL = 2,
};

// Base port.
uint32 s_portBase = 0x378;

void LPT_SetBasePort (uint32 port)
{
	s_portBase = port;
}

void LPT_SetData (uint8 dataBits)
{
	Out32 (s_portBase+REG_DATA, (short)dataBits);
}

void LPT_SetControl (uint8 controlBits)
{
	Out32 (s_portBase+REG_CONTROL, (controlBits & 0xf));
}

uint8 LPT_GetControl ()
{
	return (Inp32 (s_portBase+REG_CONTROL) & 0xf);
}

uint8 LPT_GetStatus ()
{
	return ((uint8)Inp32(s_portBase+REG_STATUS)) & 0xF8; // Ignore the 3 unused bits.
}
