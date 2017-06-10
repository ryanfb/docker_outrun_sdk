#ifndef __MENU_H__
#define __MENU_H__

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#include <stdint.h>
#include <stdbool.h>
#include "input.h"

typedef bool (MenuCallback(InputButton, uint16_t));

typedef struct
{
	const char* name;
	MenuCallback* pCallback;
	uint16_t param;
} MenuItem;

typedef struct
{
	uint8_t nItems;
	const MenuItem* pItems;
	uint8_t iSelection;
	uint8_t iActiveItem; // Single item that returned true from the callback.
	uint8_t iLeft, iTop; // Menu Position.
} MenuState;

// Initialize and draw the initial menu state.
extern void MENU_Init (MenuState* pMenuState, const MenuItem* pItems, uint8_t nItems, uint8_t left, uint8_t top, uint8_t iActiveItem);

// Update the menu state, draw changes, and handle item callbacks.
extern void MENU_Update (MenuState* pMenuState, const InputState* pInputState);

#ifdef __cplusplus
} // extern "C"
#endif // __cplusplus

#endif // __MENU_H__
