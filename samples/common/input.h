#ifndef __INPUT_H__
#define __INPUT_H__

#ifndef CPU0
#error "This file deals specifically with the subsystems accessible to CPU 0 and cannot be used from CPU 1!"
#endif

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#include <stdbool.h>
#include <stdint.h>

typedef enum
{
	// These correspond to AnalogInputPort.
	Axis_Steering = 0,
	Axis_Accelerator = 1,
	Axis_Brake = 2,
	
	MAX_AXES
} InputAxis;

typedef enum
{
	// These match to physical button bits.
	Button_Test = 1,
	Button_Service = 2,
	Button_Start = 3,
	Button_Gear = 4,
	Button_Turbo = 5,
	Button_Coin1 = 6,
	Button_Coin2 = 7,
	
	// These are derived from the axes above.
	Button_Left = 8,
	Button_Right = 9,
	Button_Accelerate = 10,
	Button_Brake = 11,
	
	MAX_BUTTONS
} InputButton;

typedef struct
{
	uint8_t  AxisValues[MAX_AXES];
	uint16_t ButtonsDown;
	uint16_t ButtonsPressed;
	uint16_t ButtonsChanged;
	uint8_t  TimeOutValues[MAX_BUTTONS];
} InputState;

// Resets the input state.
void INPUT_Init (InputState* pInputState);

// Reads digital and analog inputs, and updates the input state, including autorepeat and treating axes as buttons.
// Repeat timer tweaked assumed to be called at 60fps (once each frame).
void INPUT_Update (InputState* pInputState);

// Helpers to query the current state.
static inline bool    INPUT_IsButtonHeld    (const InputState* pInputState, InputButton button) { return (pInputState->ButtonsDown & (1 << button)) != 0; }
static inline bool    INPUT_IsButtonPressed (const InputState* pInputState, InputButton button) { return (pInputState->ButtonsPressed & (1 << button)) != 0; }
static inline uint8_t INPUT_GetAxisValue    (const InputState* pInputState, InputAxis axis) { return pInputState->AxisValues[axis]; }
static inline bool    INPUT_IsButtonPressedNoRepeat (const InputState* pInputState, InputButton button) { return (pInputState->ButtonsDown & pInputState->ButtonsChanged & (1 << button)) != 0; }

#ifdef __cplusplus
} // extern "C"
#endif // __cplusplus

#endif // __INPUT_H__
