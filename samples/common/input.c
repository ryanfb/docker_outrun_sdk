#include "input.h"
#include <io.h>

void INPUT_Init (InputState* pInputState)
{
	pInputState->ButtonsDown = 0;
	pInputState->ButtonsPressed = 0;
	pInputState->ButtonsChanged = 0;
}

// Call each frame.
void INPUT_Update (InputState* pInputState)
{
	// Read analog axes (blocking)
	for (uint8_t i=0; i<MAX_AXES; i++)
	{
		INPUT_TriggerAnalogInputSample ((AnalogInputPort)i);
		pInputState->AxisValues[i] = INPUT_ReadAnalogInput(true);
	}
	
	// Read digital buttons.
	uint16_t buttonState = INPUT_ReadDigital(DIGITAL_INPUT_1) ^ 0xff;

	// Emulate digital buttons.
	if (pInputState->AxisValues[Axis_Steering] < 0x60)
		buttonState |= (1 << Button_Left);
	if (pInputState->AxisValues[Axis_Steering] > 0xa0)
		buttonState |= (1 << Button_Right);
	if (pInputState->AxisValues[Axis_Accelerator] >= 0x40)
		buttonState |= (1 << Button_Accelerate);
	if (pInputState->AxisValues[Axis_Brake] >= 0x40)
		buttonState |= (1 << Button_Brake);
	
	uint16_t buttonsChanged = (pInputState->ButtonsDown ^ buttonState);
	pInputState->ButtonsDown = buttonState;
	pInputState->ButtonsChanged = buttonsChanged;
	
	uint16_t buttonsPressed = 0;
	uint16_t buttonMask = 1;
	for (uint8_t i=0; i<MAX_BUTTONS; i++)
	{
		if (buttonsChanged & buttonState & buttonMask)
		{
			// This button was just pressed.
			pInputState->TimeOutValues[i] = 15;
			buttonsPressed |= buttonMask;
		}
		else if (buttonState & buttonMask)
		{
			// Button is being held.
			if (--pInputState->TimeOutValues[i] == 0)
			{
				// Triggered repeat.
				pInputState->TimeOutValues[i] = 3;
				buttonsPressed |= buttonMask;
			}
		}
		
		buttonMask <<= 1;
	}
	
	pInputState->ButtonsPressed = buttonsPressed;
}
