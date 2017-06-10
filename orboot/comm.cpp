#include <Platform.h>
#include "comm.h"
#include "lpt.h"

// Timing settings.
static uint64 s_txDelay = 50000; // 50 microseconds (25 might work too).
static uint32 s_debugDelay = 0;  // Disabled by default.
static uint32 s_rxTimeOut = 0;   // Disabled by default.
static uint32 s_txTimeOut = 0; 	 // Disabled by default.

// Control register mirror. Is initialized when COMM_Reset is called.
static uint8 s_controlMirror = 0;

// Inactivity time before we go into a low power slack state.
static const uint32 kSlackOffTimeMS = 50;

// Timing control functions.
void COMM_SetTXDelayNS (uint64 nanoseconds) { s_txDelay = nanoseconds; }
void COMM_SetDebugDelayMS (uint32 milliseconds) { s_debugDelay = milliseconds; }
void COMM_SetRXTimeOutMS (uint32 milliseconds) { s_rxTimeOut = milliseconds; }
void COMM_SetTXTimeOutMS (uint32 milliseconds) { s_txTimeOut = milliseconds; }

static uint8 s_controlInversionMask = 0;
void COMM_SetControlInversionMask (uint8 mask) { s_controlInversionMask = (mask & 0xf); }

// Single threaded initialization!
static DWORD s_commThreadID = 0;
static LARGE_INTEGER s_qpcFreq = { 0 };
void COMM_Init ()
{
	s_commThreadID = GetCurrentThreadId();
	DWORD_PTR prevMask = SetThreadAffinityMask (GetCurrentThread(), 1);
	BOOL qpfSucceeded = QueryPerformanceFrequency (&s_qpcFreq); // Well, this shouldn't fail.
	DEBUG_ASSERT(qpfSucceeded && s_qpcFreq.QuadPart);
}

// This will sleep (well, busy wait actually) for at least the given amount of nanoseconds.
static void SleepNanoSeconds (uint64 nanoSecs)
{
	if (!nanoSecs)
		return;

	// We use QueryPerformanceCounter to do actual sleeping since it's about as accurate as we will get.
	LARGE_INTEGER count1, count2;
	QueryPerformanceCounter (&count1);
	for (;;)
	{
		QueryPerformanceCounter (&count2);
		uint64 nanoSecsElapsed = (((count2.QuadPart - count1.QuadPart) * 1000000000) / s_qpcFreq.QuadPart);
		if (nanoSecsElapsed >= nanoSecs)
			break;
	}
}

// Resets the control register.
void COMM_Reset ()
{
	// Make sure we're doing it from the same thread as we called Init() on.
	DEBUG_ASSERT(GetCurrentThreadId() == s_commThreadID);

	// Default: we want nSTROBE and nAUTOFEED to be high. Both are inverted.
	s_controlMirror = 0;
	if (s_debugDelay)
		Sleep (s_debugDelay);
	LPT_SetControl (s_controlMirror ^ s_controlInversionMask);
	LPT_SetData (0);
	SleepNanoSeconds (s_txDelay);
}

static uint8 SwizzleStatus08E (uint8 status)
{
	// Since we reversed the bit order on the A20..A23 pins on the outrun PCB, 
	// We should switch the corresponding status bits here.
	//
	// +-------------+-----------+----------------------------+----------+-------------+
	// | Outrun PCB  | 0.8E Lite |          LPT               | Register | Nibble Mode |
	// +-----+-------+-----------+-----+----------+-----------+----------+-------------+
	// | A23 | Bit 0 | Inverted  | P11 | Inverted | BUSY      | Status:7 |   Bit 3/7   |
	// | A22 | Bit 1 | Inverted  | P12 |     -    | PAPER OUT | Status:5 |   Bit 2/6   |
	// | A21 | Bit 2 | Inverted  | P13 |     -    | SELECT    | Status:4 |   Bit 1/5   |
	// | A20 | Bit 3 | Inverted  | P15 |     -    | nERROR    | Status:3 |   Bit 0/4   |
	// | A19 | Bit 4 | Inverted  | P10 |     -    | nACK      | Status:6 |   PtrClk    |
	// +-----+-------+-----------+-----+----------+-----------+----------+-------------+
	//
	// So, register bits 4 and 5 should be swapped, and bits 7 and 3 - they should be inverted too.
	uint8 swizzled = (((status & 0x8)  << 4) ^ 0x80) |
		              ((status & 0x10) << 1) | 
					  ((status & 0x20) >> 1) |
					   (status & 0x40) | // nACK stays.
					 (((status & 0x80) >> 4) ^ 0x8);

	return swizzled;
}

static bool WaitForStatusMask (uint32 timeOut, uint8 maskHigh, uint8 maskLow, uint8& status)
{
	// Wait for the BUSY line to go (or be) low.
	uint32 timerStart = GetTickCount();
	bool bSlackOff = false;
	for (;;)
	{
		status = SwizzleStatus08E (LPT_GetStatus());
		if ((status & maskHigh) == maskHigh && 
			(status & maskLow) == 0)
			return true;

		// If we're not in slack mode already, get the current time.
		// Also if we have a timeout set.
		uint32 curTime;
		if (timeOut || !bSlackOff)
			curTime = GetTickCount();

		if (timeOut && (curTime - timerStart) > s_txTimeOut)
			return false; // Timeout.

		// Slack?
		if (!bSlackOff)
			bSlackOff = ((curTime - timerStart) >= kSlackOffTimeMS);
		if (bSlackOff)
			Sleep(10);
	}

	return true;
}

// Writes a single byte in compatibility mode. Returns true if sent and acknowledged, false on timeout.
bool COMM_SendByte (uint8 data)
{
	// Make sure we're doing it from the same thread as we called Init() on.
	DEBUG_ASSERT(GetCurrentThreadId() == s_commThreadID);

	// Wait for BUSY to be low (indicates the hardware can receive data).
	uint8 status;
	if (!WaitForStatusMask (s_txTimeOut, STATUS_BUSY_i, 0, status))
		return false;

	// Device not busy - set data on the output pins.
	if (s_debugDelay)
		Sleep (s_debugDelay);
	LPT_SetData (data);

	// Wait before we strobe. We have to do this or the data might be read wrong.
	SleepNanoSeconds (s_txDelay);

	// Pull the strobe line to low.
	if (s_debugDelay)
		Sleep (s_debugDelay);
	s_controlMirror |= CONTROL_nSTROBE_i;
	LPT_SetControl (s_controlMirror ^ s_controlInversionMask);

	// Wait for BUSY high.
	if (!WaitForStatusMask (s_txTimeOut, 0, STATUS_BUSY_i, status))
		return false;

	// Set the strobe line back to high.
	if (s_debugDelay)
		Sleep (s_debugDelay);
	s_controlMirror &= ~CONTROL_nSTROBE_i;
	LPT_SetControl (s_controlMirror ^ s_controlInversionMask);

	// Ignore nACK transition, since we're not sending anything back anyway.
	return true;
}

static uint8 StatusToNibble (uint8 statusByte) 
{ 
	return ((statusByte & (STATUS_nERROR|STATUS_SELECT|STATUS_PAPEROUT)) >> 3)
		| (((statusByte & STATUS_BUSY_i) ^ STATUS_BUSY_i) >> 4); // Busy is inverted.
}

// Reads a nibble.
static int8 ReadNibble ()
{
	// Set HostBusy (=nAUTOFEED) to low. This tells we are ready to receive a byte.
	if (s_debugDelay)
		Sleep (s_debugDelay);
	s_controlMirror |= CONTROL_nAUTOFEED_i;
	LPT_SetControl (s_controlMirror ^ s_controlInversionMask);

	// Device will set nibble data on the status bits; then assert PtrClk (=nACK) low.
	uint8 status;
	if (!WaitForStatusMask (s_rxTimeOut, 0, STATUS_nACK, status))
		return -1;

	// Wait a bit and read the status bits again, just to be sure that they weren't still being set.
	SleepNanoSeconds (10);
	uint8 nibbleData = StatusToNibble (SwizzleStatus08E (LPT_GetStatus()));

	// Set HostBusy (=nAUTOFEED) to high.
	if (s_debugDelay)
		Sleep (s_debugDelay);
	s_controlMirror &= ~CONTROL_nAUTOFEED_i;
	LPT_SetControl (s_controlMirror ^ s_controlInversionMask);

	// Wait for PtrClk (=nACK) to go high again.
	if (!WaitForStatusMask (s_rxTimeOut, STATUS_nACK, 0, status))
		return -1;

	return nibbleData;
}

// Reads a single byte, in two nibbles.
int16 COMM_RecvByte ()
{
	// Make sure we're doing it from the same thread as we called Init() on.
	DEBUG_ASSERT(GetCurrentThreadId() == s_commThreadID);

	int8 lowBits = ReadNibble();
	if (lowBits == -1)
		return -1;

	int8 highBits = ReadNibble();
	if (highBits == -1)
		return -1;

	return (((uint8)highBits) << 4) | ((uint8)lowBits);
}
