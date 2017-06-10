#ifndef __PLATFORM_H__
#define __PLATFORM_H__

#include <windows.h>
#include <assert.h>

// Types.
typedef char int8;
typedef unsigned char uint8;
typedef short int16;
typedef unsigned short uint16;
typedef int int32;
typedef unsigned int uint32;
typedef unsigned __int64 uint64;

#define DEBUG_ASSERT assert

#endif // __PLATFORM_H__
