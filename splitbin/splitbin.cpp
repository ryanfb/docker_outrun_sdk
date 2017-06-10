#include <Platform.h>

int main (int argc, char **argv)
{
	if (argc < 5)
	{
		printf ("SplitBin - Splits a binary image in to separate (interleaved) (EP)ROM images.\n");
		printf ("Usage: splitbin.exe <inputfile> <rom size> <interleave> <name1> [<name2> ...]\n");
		printf ("Example: splitbin.exe maincpu.bin 65536 2 epr-10380b.133 epr-10382b.118\n");
		printf ("Note: The file names should be supplied in interleaved order.\n");
		return 0;
	}

	const char* inputFile = NULL; 
	uint64 romSize = 0, interleave = 0;
	List<const char*> romNames;
	int argIdx = 1;
	while (argIdx < argc)
	{
		const char* arg = argv[argIdx];
		if (arg[0] == '-')
		{
			// Handle options here.
			printf ("Invalid option: '%s'.\n", arg);
			return 1;
		}
		else if (!inputFile)
		{
			inputFile = arg;
		}
		else if (!romSize)
		{
			if (sscanf (arg, "%I64u", &romSize) != 1 || romSize == 0)
			{
				printf ("Invalid ROM size: '%s'.\n", arg);
				return 1;
			}
		}
		else if (!interleave)
		{
			if (sscanf (arg, "%I64u", &interleave) != 1 || interleave == 0)
			{
				printf ("Invalid interleave value: '%s'.\n", arg);
				return 1;
			}
		}
		else // Output file names.
		{
			romNames.Append(arg);
		}
		argIdx++;
	}

	if (romNames.Empty())
	{
		printf ("No output file names specified.\n");
		return 1;
	}

	if ((romNames.Size() % interleave) != 0)
	{
		printf ("Mismatching number of output file names; expected a multiple of the interleave value.\n");
		return 1;
	}

	HANDLE hFile = CreateFile (inputFile, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		printf ("Input file '%s' not found.\n", inputFile);
		return 1;
	}

	uint64 maxFileSize = romSize * romNames.Size();
	LARGE_INTEGER fileSize;
	if (!GetFileSizeEx (hFile, &fileSize))
	{
		printf ("Can't get file size for input file '%s'.\n", inputFile);
		return 1;
	}

	if ((uint64)fileSize.QuadPart > maxFileSize)
	{
		printf ("Not enough output file names to accomodate input file size of %I64u.\n", fileSize.QuadPart);
		return 1;
	}

	uint8* data = new uint8[(size_t)(romSize * interleave)];
	uint8* romData = new uint8[(size_t)romSize];
	List<const char*>::ConstCursor c = romNames.First();
	DWORD dataSize = 0;
	for (;;)
	{
		if (!ReadFile (hFile, data, (DWORD)(romSize*interleave), &dataSize, NULL))
		{
			printf ("Error reading input file '%s'.\n", inputFile);
			CloseHandle (hFile);
			return 1;
		}

		for (uint64 j=0; j<interleave; j++)
		{
			uint64 romUsage = 0;
			for (uint64 i=j; i<dataSize; i+=interleave)
				romData[romUsage++] = data[i];
			for (uint64 i=romUsage; i<romSize; i++)
				romData[i] = 0; // Zero out the rest.

			HANDLE hOutFile = CreateFile (c.GetValue(), GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, NULL);
			if (hOutFile == INVALID_HANDLE_VALUE)
			{
				printf ("Error creating output file '%s' (%u).", c.GetValue(), GetLastError());
				CloseHandle (hFile);
				return 1;
			}

			DWORD bytesWritten;
			if (!WriteFile (hOutFile, romData, (DWORD)romSize, &bytesWritten, NULL) || romSize != bytesWritten)
			{
				printf ("Error writing to output file '%s' (%u).", c.GetValue(), GetLastError());
				CloseHandle (hOutFile);
				CloseHandle (hFile);
				return 1;
			}

			CloseHandle (hOutFile);
			printf ("Written '%s'.\n", c.GetValue());
			c++;
		}

		if (dataSize < romSize*interleave)
			break; // EOF, done.
	}

	delete[] data;
	delete[] romData;

	CloseHandle (hFile);
	return 0;
}
