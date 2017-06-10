#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include "palette.h"
#include "text.h"
#include "tile.h"
#include "sprite.h"
#include <io.h>
#include <irq.h>
#include "memtest.h"
#include "hwinit.h"
#include <stdlib.h>

// CRC-32 Functions
static void BuildCrcTable (unsigned long *crc32table)
{
    unsigned long c;
    int n, k;
    unsigned long poly;                 /* polynomial exclusive-or pattern */

    static const unsigned char p[] = {0,1,2,4,5,7,8,10,11,12,16,22,23,26};

	/* make exclusive-or pattern from polynomial (0xedb88320UL) */
    poly = 0xedb88320UL;
//    for (n = 0; n < sizeof(p)/sizeof(unsigned char); n++)
//        poly |= 1UL << (31 - p[n]);

	/* generate a crc for every 8-bit value */
    for (n = 0; n < 256; n++) {
        c = (unsigned long)n;
        for (k = 0; k < 8; k++)
            c = c & 1 ? poly ^ (c >> 1) : c >> 1;
        crc32table[n] = c;
    }
}

static unsigned long s_crcTable[256];

unsigned long CalcCRC32 (void* pData, unsigned int nBytes, unsigned char nStride)
{
	unsigned long crc = 0xffffffff;
	unsigned char* pByteData = (unsigned char*)pData;
	while (nBytes)
	{
		crc = s_crcTable[((int)crc ^ (*pByteData)) & 0xff] ^ (crc >> 8);
		pByteData += nStride;
		--nBytes;
	}
	return crc ^ 0xffffffff;
}

// List of known ROMs.
const struct ROMINFO
{
	const char* name;
	unsigned short ic;
	unsigned long crc;
} romInfo[] = 
{
	// Main, Rev. B
	{ "EPR-10380b", 133, 0x1f6cadad },
	{ "EPR-10381b", 132, 0xbe8c412b },
	{ "EPR-10382b", 118, 0xc4c3fa1a },
	{ "EPR-10383b", 117, 0x10a2014a },

	// Sub-cpu.
	{ "EPR-10327a", 76,  0xe28a5baf },
	{ "EPR-10329a", 58,  0xda131c81 },
	{ "EPR-10328a", 75,  0xd5ec5e5d },
	{ "EPR-10330a", 57,  0xba9ec82a },
	
	{ "Empty.Mame",  0,  0xd7978eeb },
	{ "Empty.Real",  0,  0xdeab7e4e },
};

typedef void RESTORECALLBACK();

void RestorePalette ()
{
	TEXT_InitDefaultPalette();
	
	// Clear road palette.
	const uint16_t color = 0;
	for (uint16_t i=0x400; i<0x410; i++)
		PALETTE_SetColor16 (i, color);
	
	for (uint16_t i=0x420; i<0x440; i++)
		PALETTE_SetColor16 (i, color);

	// Background colors (scanline fill mode).
	for (uint16_t i=0; i<128; i++)
		PALETTE_SetColor16 (i+0x780, color);
}

const struct RAMINFO
{
	const char*    name;
	unsigned long  address;
	unsigned short interleave;
	unsigned long  size;
	RESTORECALLBACK* pRestoreCallback;
} ramInfo[] = 
{
//	{ "Main0", 0x60000, 1, 2, 0x2000 },
//	{ "Main1", 0x60001, 1, 2, 0x2000 },
//	{ "Main2", 0x62000, 1, 2, 0x2000 },
//	{ "Main3", 0x62001, 1, 2, 0x2000 },


	// IC95 is een multiplexert/selector geval. SAB17..19 (1 t/m 19 op de CPU). LDS en UDS hebben we (byte select).
	// Ok dus CPU heeft gewoon geen A0. Nice. LDS=bit 0..7; UDS=bit 8..15.
	// 128k per rom bank (2 chips). Gebruikt bits 0..16 (op de M68k)
	// 
	//               LDS(DB 0..7)   UDS(DB 8..15)
	// 0x0200000     IC58           IC76
	// 0x0220000     IC57           IC75
	// 0x0240000     IC56           IC74
	// Als A17 en A18 aan (0x60000), dan gaan we naar RAM. A14 hoog -> !RAM1 hoog. !RAM1 -> !CE -> IC114 aan.
	//
	// Subram = 2x8k, dus extra selector op SAB14.
	// 0x0260000     IC54           IC72
	// 0x0220000     IC55(SA14=CE)  IC73(SA14=CE)	// Inverter (LS04 IC??) sets !CE to the inverse.

	// Big endian 16 bit data locations:
	// A 16-bit read to address #0 will load the high (first) byte at position 0 over lines D8..15, and the low byte at position 1 from lines D0..7.

	// SUB RAM.
	// 54 is the lower half due to the inverter (LS04) A14 to CE.
	// *** Note that sub ram needs to be untouched by the secondary CPU (including interrupts) during testing! ***
	{ "Sub IC72",  0x260000, 2, 0x4000 }, // 32k sub ram; 4 chips. ic 73+72(db8..15); 55+54(db0..7); 
	{ "Sub IC54",  0x260001, 2, 0x4000 },
	{ "Sub IC73",  0x264000, 2, 0x4000 },
	{ "Sub IC55",  0x264001, 2, 0x4000 },

	{ "Til IC64",  0x100000, 2, 0x10000 }, // 16 x 64*32 * 2 = 64k
	{ "Til IC62",  0x100001, 2, 0x10000, TILE_Reset },
//	{ "Text0   ",  0x110000, 2, 0x1000 }, // 4K text ram
//	{ "Text1   ",  0x110001, 2, 0x1000 },
	{ "Pal IC92",  0x120000, 2, 0x2000 }, // 8KB palette ram. Kun je hier uberhaupt wel halve words uit lezen? Of fixt die 68k dat intern?
	{ "Pal IC95",  0x120001, 2, 0x2000, RestorePalette },

	// Fixme: check IC #s.
	{ "Spr0    ",  0x130000, 2, 0x800 }, // Has a flip control, so this only tests half (could flip in a callback)
	{ "Spr1    ",  0x130001, 2, 0x800 },
};

static const char* itoa (int i)
{
	bool isNegative = i < 0;
	uint32_t positive;
	if (isNegative)
		positive = ((i ^ 0xffffffff) + 1);
	else positive = i;
	
	static char buf[10]; // Very single threaded.
	char* write = buf + sizeof(buf);
	*(--write) = '\0';
	
	do
	{
		// Use 'div' to get both the quotient and the remainder.
		div_t result = div (positive, 10);
		*(--write) = '0' + result.rem;
		positive = result.quot;
	} while (positive);
	
	if (isNegative)
		*(--write) = '-';
	return write;
}

void ValidateRom (void* address, unsigned short ic)
{
	unsigned int i;
	unsigned long crc;
	
	TEXT_SetColor (TEXT_White);

	TEXT_Write ("IC ");
	TEXT_Write (itoa(ic));
	TEXT_Write (" ");

	crc = CalcCRC32 (address, 65536, 2);
	for (uint16_t i=0; i<sizeof(romInfo)/sizeof(struct ROMINFO); i++)
	{
		if (crc == romInfo[i].crc)
		{
			// Found a match.
			TEXT_SetColor ((romInfo[i].ic == ic) ? TEXT_Green : romInfo[i].ic ? TEXT_Red : TEXT_Purple);
			TEXT_Write (romInfo[i].name);
			TEXT_Write ("\n");
			return;
		}
	}

	// Not found.
	TEXT_SetColor (TEXT_Gray);
	TEXT_WriteHex32 (crc);
	TEXT_Write ("\n");
}
		
int main (void)
{
	HW_Init (HWINIT_Default, 0);

	TEXT_InitDefaultPalette();

	TEXT_SetWindow (26, 0, 45, 28, false);
	TEXT_SetColor (TEXT_Cyan);
	TEXT_Write ("ROM TEST\n\n");
	
	unsigned int crc;
	BuildCrcTable (s_crcTable);

	ValidateRom ((void*)0x0, 133);
	ValidateRom ((void*)0x1, 118);
	ValidateRom ((void*)0x20000, 132);
	ValidateRom ((void*)0x20001, 117);
	ValidateRom ((void*)0x40000, 131);
	ValidateRom ((void*)0x40001, 116);

	TEXT_SetWindow (45, 2, 64, 28, false);
	ValidateRom ((void*)0x200000, 76);
	ValidateRom ((void*)0x200001, 58);
	ValidateRom ((void*)0x220000, 75);
	ValidateRom ((void*)0x220001, 57);
	ValidateRom ((void*)0x240000, 74);
	ValidateRom ((void*)0x240001, 56);

	TEXT_SetWindow (26, 0, 64, 28, true);

//	*((volatile unsigned short*)0x260000) = 0x1234; // Test.
//	for(;;);
	// Ok, bigendian = 0x260000 = 0x12; 0x260001 = 0x34.
	// Benieuwd eigenlijk hoe ze die 8 bit writes dan doen. Wat gebeurt er met address bit 0?

//	DEBUG_ASSERT(123 < 0);

//	SendString ("Done validating roms.\n");

	// RAM TEST
	TEXT_GotoXY (0, 9);
	TEXT_SetColor (TEXT_Cyan);
	TEXT_Write ("RAM TEST\n\n");

	for (uint16_t i=0; i<sizeof(ramInfo)/sizeof(struct RAMINFO); i++)
	{
		bool bFailed = false;
		TEXT_SetColor (TEXT_White);
		TEXT_Write (ramInfo[i].name);
		TEXT_Write (" DB");
		datum dbTest = memTestDataBus8 ((volatile datum*)ramInfo[i].address);
		if (dbTest)
		{
			TEXT_SetColor (TEXT_Red);
			TEXT_Write (" ERROR ");
			TEXT_SetColor (TEXT_Yellow);
			TEXT_WriteHex((int)dbTest, 2, '0');
			
			bFailed = true;
		}
		else 
		{
			TEXT_SetColor (TEXT_Green);
			TEXT_Write (" OK");
		}

		if (!bFailed)
		{
			TEXT_SetColor (TEXT_White);
			TEXT_Write (" AB");
			datum* addrTest = memTestAddressBus ((volatile datum*)ramInfo[i].address, ramInfo[i].size, 2); // Fixme: grab from list.
			if (addrTest)
			{
				TEXT_SetColor (TEXT_Red);
				TEXT_Write (" ERROR ");
				TEXT_SetColor (TEXT_Yellow);
				TEXT_WriteHex((int)addrTest, 6, '0');

				bFailed = true;
			}
			else 
			{
				TEXT_SetColor (TEXT_Green);
				TEXT_Write (" OK");
			}
		}

		if (!bFailed)
		{
			TEXT_SetColor (TEXT_White);
			TEXT_Write (" DEV");

			datum* devTest = memTestDevice ((volatile datum*)ramInfo[i].address, ramInfo[i].size, ramInfo[i].interleave);
			if (devTest)
			{
				TEXT_SetColor (TEXT_Red);
				TEXT_Write (" ERROR ");
				TEXT_SetColor (TEXT_Yellow);
				TEXT_WriteHex((int)devTest, 6, '0');
			}
			else 
			{
				TEXT_SetColor (TEXT_Green);
				TEXT_Write (" OK");
			}
		}

		// Clean up what we just broke.
		if (ramInfo[i].pRestoreCallback)
			ramInfo[i].pRestoreCallback ();
		
		TEXT_Write ("\n");
	}

	// Here we go.
	for (;;)
	{
		TEXT_GotoXY (0, 21);

//		TEXT_SetColor (TEXT_White);
//		TEXT_Write ("crc ");
//		TEXT_SetColor (TEXT_Yellow);
//		TEXT_WriteHex32 (crc);

		TEXT_SetColor (TEXT_Purple);
		TEXT_Write ("\nirq4 ");
		TEXT_SetColor (TEXT_Yellow);
		TEXT_WriteHex32 (IRQ4_GetCounter());

//		// Schrijf shit naar port c.
//		*((unsigned char*)0x140005) = ((henk&256)? 0x21: 0); // 2=cont. 40=sprite?
		IRQ4_Wait ();
	}
}
