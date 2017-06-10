#include <Platform.h>
#include <stdio.h>
#include <conio.h>
#include "lpt.h"
#include "comm.h"

static const char versionString[] = "0.9A";

enum
{
	COMMAND_NOP        = 0,
	COMMAND_REBOOTRAM  = 1,
	COMMAND_REBOOTROM  = 2,
	COMMAND_UPLOADMAIN = 3,
	COMMAND_UPLOADSUB  = 4,
};

bool Nop ()       { return COMM_SendByte (COMMAND_NOP); }
bool RebootRAM () { return COMM_SendByte (COMMAND_REBOOTRAM); }
bool RebootROM () { return COMM_SendByte (COMMAND_REBOOTROM); }

// Write a 16 bit value in high-low (bigendian) order.
static bool COMM_SendWord (uint16 word)
{
	return COMM_SendByte (word >> 8) && COMM_SendByte (word & 0xff);
}

static bool UploadInternal (const uint8* pData, uint16 nBytes)
{
	// Command has been sent.
	if (!COMM_SendWord (nBytes))
		return false;

	// Now send out all the data.
	for (uint32 i=0; i<nBytes; i++)
		if (!COMM_SendByte(pData[i]))
			return false;

	return true;
}

bool UploadMainRam (const void* pData, uint16 nBytes)
{
	DEBUG_ASSERT(pData);
	DEBUG_ASSERT(nBytes <= 32768);
	if (!COMM_SendByte (COMMAND_UPLOADMAIN))
		return false;

	return UploadInternal ((const uint8*)pData, nBytes);
}

bool UploadSubRam (const void* pData, uint16 nBytes)
{
	DEBUG_ASSERT(pData);
	DEBUG_ASSERT(nBytes <= 32768);
	if (!COMM_SendByte (COMMAND_UPLOADSUB))
		return false;

	return UploadInternal ((const uint8*)pData, nBytes);
}

// Options:
// -port <lpt port>; default 0x378.
// -txdelay <ns>; default 50000.
// -txtimeout <ms>; default 0 (disabled).
// -debugdelay (ms); default 0 (disabled).
// -console; reads debug output instead of exiting.

int main (int argc, char** argv)
{
	printf ("Outrun Bootloader %s.\n", versionString);

	// Parse options.
	bool bConsoleSet = false;
	bool bPortSet = false;
	bool bTXDelaySet = false;
	bool bTXTimeOutSet = false;
	bool bDebugDelaySet = false;

	// Arguments.
	const char* mainRamImage = NULL;
	const char* subRamImage = NULL;

	for (int32 i=1; i<argc; i++)
	{
		const char* arg = argv[i];
		if (arg[0] == '-' || arg[0] == '/')
		{
			// Option.
			arg++;
			if (stricmp (arg, "port") == 0)
			{
				if (bPortSet)
				{
					printf ("Port already set!\n");
					return 1;
				}
				else if (i+1==argc)
				{
					printf ("Port option needs argument!\n");
					return 1;
				}
				else
				{
					i++;
					uint32 port;
					if (sscanf (argv[i], "0x%x", &port) == 1 || 
						sscanf (argv[i], "0x%X", &port) == 1 || 
						sscanf (argv[i], "%u",   &port) == 1)
					{
						if (port == 378) 
							printf ("Warning: did you mean to use hexadecimal (0x378) notation?\n");
						LPT_SetBasePort (port);
						bPortSet = true;
					}
					else
					{
						printf ("Port option needs a valid positive value!\n");
						return 1;
					}
				}
			}
			else if (stricmp (arg, "txdelay") == 0)
			{
				if (bTXDelaySet)
				{
					printf ("TX delay already set!\n");
					return 1;
				}
				else if (i+1==argc)
				{
					printf ("TX delay option needs argument!\n");
					return 1;
				}
				else
				{
					i++;
					uint64 txDelay;
					if (sscanf (argv[i], "%I64u", &txDelay) == 1)
					{
						COMM_SetTXDelayNS (txDelay);
						bTXDelaySet = true;
						if (txDelay < 25000)
							printf ("Warning: TX delay should be at least 35000 nanoseconds.\n");
					}
					else 
					{
						printf ("TX delay option needs a valid positive value!\n");
						return 1;
					}
				}
			}
			else if (stricmp (arg, "txtimeout") == 0)
			{
				if (bTXTimeOutSet)
				{
					printf ("TX timeout already set!\n");
					return 1;
				}
				else if (i+1==argc)
				{
					printf ("TX timeout option needs argument!\n");
					return 1;
				}
				else
				{
					i++;
					uint32 txTimeOut;
					if (sscanf (argv[i], "%u", &txTimeOut) == 1)
					{
						COMM_SetTXTimeOutMS (txTimeOut);
						bTXTimeOutSet = true;
					}
					else
					{
						printf ("TX timeout option needs a valid positive value!\n");
						return 1;
					}
				}
			}
			else if (stricmp (arg, "debugdelay") == 0)
			{
				if (bDebugDelaySet)
				{
					printf ("Debug delay already set!\n");
					return 1;
				}
				else if (i+1==argc)
				{
					printf ("Debug delay option needs argument!\n");
					return 1;
				}
				else
				{
					i++;
					uint32 dbgDelay;
					if (sscanf (argv[i], "%u", &dbgDelay) == 1)
					{
						COMM_SetDebugDelayMS (dbgDelay);
						bDebugDelaySet = true;
					}
					else
					{
						printf ("Debug delay option needs a valid positive value!\n");
						return 1;
					}
				}
			}
			else if (stricmp (arg, "console") == 0)
			{
				if (bConsoleSet)
				{
					printf ("Console parameter already specified!\n");
					return 1;
				}
				else bConsoleSet = true;
			}
		}
		else
		{
			// Argument.
			if (!mainRamImage)
				mainRamImage = argv[i];
			else if (!subRamImage)
				subRamImage = argv[i];
			else
			{
				printf ("Too many arguments! Only a main and sub image are required.\n");
				return 1;
			}
		}
	}

	if (!mainRamImage)
	{
		// Print options.
		printf ("Usage: orboot [-options] main.bin [sub.bin]\n");
		printf ("Options: -port        Set the LPT port number (default: 0x378).\n");
		printf ("         -txdelay     Sets the strobe delay in nanoseconds (default: 50000).\n");
		printf ("         -debugdelay  Sets a debug delay in milliseconds between transitions.\n");
		printf ("         -txtimeout   Sets a timeout delay in milliseconds (0=disabled).\n");
		printf ("         -console     Keeps the console open and prints nibble mode output.\n");
		return 1;
	}

	if (!bTXTimeOutSet)
		COMM_SetTXTimeOutMS (10*1000); // 10 second default timeout.

	// Load images from disk into memory.
	HANDLE hMain = CreateFile (mainRamImage, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
	if (hMain == INVALID_HANDLE_VALUE)
	{
		printf ("Main ram image '%s' not found!\n", mainRamImage);
		return 1;
	}

	LARGE_INTEGER mainRamSize;
	if (!GetFileSizeEx (hMain, &mainRamSize) || mainRamSize.QuadPart > 32*1024)
	{
		printf ("Invalid size for main ram image '%s'!\n", mainRamImage);
		CloseHandle (hMain);
		return 1;
	}

	uint8* mainRamData = new uint8[(size_t)mainRamSize.QuadPart];
	DWORD bytesRead;
	if (!ReadFile (hMain, mainRamData, (uint32)mainRamSize.QuadPart, &bytesRead, NULL) || bytesRead != (uint32)mainRamSize.QuadPart)
	{
		printf ("Error reading main ram image '%s'!\n", mainRamImage);
		delete[] mainRamData;
		CloseHandle (hMain);
		return 1;
	}

	CloseHandle (hMain);
	hMain = INVALID_HANDLE_VALUE;

	uint8* subRamData = NULL;
	LARGE_INTEGER subRamSize = { 0 };
	if (subRamImage)
	{
		HANDLE hSub = CreateFile (subRamImage, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
		if (hSub == INVALID_HANDLE_VALUE)
		{
			printf ("Sub ram image '%s' not found!\n", subRamImage);
			delete[] mainRamData;
			return 1;
		}

		if (!GetFileSizeEx (hSub, &subRamSize) || subRamSize.QuadPart > 32*1024)
		{
			printf ("Invalid size for sub ram image '%s'!\n", subRamImage);
			delete[] mainRamData;
			CloseHandle (hSub);
			return 1;
		}

		subRamData = new uint8[(uint32)subRamSize.QuadPart];
		if (!ReadFile (hSub, subRamData, (uint32)subRamSize.QuadPart, &bytesRead, NULL) || bytesRead != (uint32)subRamSize.QuadPart)
		{
			printf ("Error reading sub ram image '%s'!\n", subRamImage);
			delete[] mainRamData;
			delete[] subRamData;
			CloseHandle (hSub);
			return 1;
		}

		CloseHandle (hSub);
	}

	// Start uploading.
	COMM_Init ();
	COMM_SetControlInversionMask (CONTROL_nAUTOFEED_i); // We have autofeed inverted on the PCB.
	COMM_Reset ();

	bool bError = false;
	printf ("Initializing...");
	if (!Nop())
	{
		printf ("\nBootloader device not in default state. Operation timed out.\n");
		bError = true;
	}
	else
	{
		// Start uploading.
		printf ("\nUploading to main ram (%d bytes)...", (uint32)mainRamSize.QuadPart);
		if (!UploadMainRam (mainRamData, (uint16)mainRamSize.QuadPart))
		{
			printf ("\nUpload to main ram timed out.\n");
			bError = true;
		}
		else
		{
			printf ("\n");
			if (subRamData)
			{
				printf ("Uploading to sub ram (%d bytes)...", (uint32)subRamSize.QuadPart);
				if (!UploadSubRam (subRamData, (uint16)subRamSize.QuadPart))
				{
					printf ("\nUpload to sub ram timed out.\n");
					bError = true;
				}
				else printf ("\n");
			}
		}
	}

	delete[] mainRamData;
	if (subRamData)
		delete[] subRamData;

	if (bError)
		return 1;

	printf ("Rebooting to ram...");
	if (!RebootRAM ())
	{
		printf ("\nReboot timed out.\n");
		return 1;
	}
	printf ("\n");

	if (bConsoleSet)
	{
		// Handle console output.
		printf ("\n");
		SetThreadPriority (GetCurrentThread(), THREAD_PRIORITY_LOWEST);

		for (;;)
		{
			int16 byte = COMM_RecvByte ();
			if (byte == -1)
			{
				printf ("\n\nRead timed out. Shouldn't happen.\n");
				return 1;
			}
			putchar (byte);
		}
	}

	return 0;
}
