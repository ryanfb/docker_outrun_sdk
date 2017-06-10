#ifndef __LPT_H__
#define __LPT_H__

#include <Platform.h>

// Basic functionality to bitwise control the LPT port.
// Uses inpout32.
//
// +-------------+--------------+------+-----+---------+---------+
// | Name        | Register:Bit | Pins | Dir | Wr. Inv | Rd. Inv |
// +-------------+--------------+------+-----+---------+---------+
// | nERROR      | STATUS:3     |  15  |  R  |    -    |   No    |
// | SELECT      | STATUS:4     |  13  |  R  |    -    |   No    |
// | PAPER OUT   | STATUS:5     |  12  |  R  |    -    |   No    |
// | nACK        | STATUS:6     |  10  |  R  |    -    |   No    |
// | BUSY        | STATUS:7     |  11  |  R  |    -    |   Yes   |
// +-------------+--------------+------+-----+---------+---------+
// | DATA0       | DATA:0       |   2  |  W  |   No    |    -    |
// | DATA1       | DATA:1       |   3  |  W  |   No    |    -    |
// | DATA2       | DATA:2       |   4  |  W  |   No    |    -    |
// | DATA3       | DATA:3       |   5  |  W  |   No    |    -    |
// | DATA4       | DATA:4       |   6  |  W  |   No    |    -    |
// | DATA5       | DATA:5       |   7  |  W  |   No    |    -    |
// | DATA6       | DATA:6       |   8  |  W  |   No    |    -    |
// | DATA7       | DATA:7       |   9  |  W  |   No    |    -    |
// +-------------+--------------+------+-----+---------+---------+
// | nSTROBE	 | CONTROL:0    |   1  | R/W |   Yes   |    ?    |
// | nAUTOFEED	 | CONTROL:1    |  14  | R/W |   Yes   |    ?    |
// | nINITIALIZE | CONTROL:2    |  16  | R/W |   No    |    ?    |
// | nSELECT PRN | CONTROL:3    |  17  | R/W |   Yes   |    ?    |
// +-------------+--------------+------+-----+---------+---------+
// | GND         |       -      | 18-25|  -  |    -    |    -    |
// +-------------+--------------+------+-----+---------+---------+
//
// Wr. Inv are measured on the DB25 output pins. 
// Not inverted means setting a bit to 1 means TTL high on the output.
//

// Sets the base port address. Default is 0x378 (LPT1).
void LPT_SetBasePort (uint32 port);

// Status bits.
enum
{
	STATUS_nERROR   = 1 << 3,
	STATUS_SELECT   = 1 << 4,
	STATUS_PAPEROUT = 1 << 5,
	STATUS_nACK     = 1 << 6,
	STATUS_BUSY_i   = 1 << 7, // Inverted! Bit high means low input.
};

// Reads the status register.
uint8 LPT_GetStatus ();

// Control bits.
enum
{
	CONTROL_nSTROBE_i        = 1 << 0, // Inverted! Bit high means low output.
	CONTROL_nAUTOFEED_i      = 1 << 1, // Inverted! Bit high means low output.
	CONTROL_nINITIALIZE      = 1 << 2, 
	CONTROL_nSELECTPRINTER_i = 1 << 3, // Inverted! Bit high means low output.
};

// Sets the control register.
void LPT_SetControl (uint8 controlBits);

// Reads from the control register.
uint8 LPT_GetControl ();

// Sets the data bits.
void LPT_SetData (uint8 dataBits);

#endif // LPT_H__
