#include "menu.h"
#include "text.h"

static void MENU_DrawItem (const MenuState* pMenuState, uint8_t itemIdx, bool isSelected, bool isActive)
{
	TEXT_GotoXY (pMenuState->iLeft, pMenuState->iTop + (itemIdx << 1));

	if (isSelected)
	{
		// Draw arrow.
		TEXT_SetColor (TEXT_Yellow); // We could blink this thing.
		TEXT_WriteRawChar (TEXTGLYPH_ARROWRIGHT);
	}
	else
	{
		// Remove arrow.
		TEXT_WriteChar (' ');
	}

	// Draw item text.
	TEXT_WriteChar (' ');
	TEXT_SetColor (isActive ? TEXT_Cyan : isSelected ? TEXT_White : TEXT_Gray);
	TEXT_Write (pMenuState->pItems[itemIdx].name);
}

void MENU_Init (MenuState* pMenuState, const MenuItem* pItems, uint8_t nItems, uint8_t left, uint8_t top, uint8_t iActiveItem)
{
	pMenuState->nItems = nItems;
	pMenuState->pItems = pItems;
	pMenuState->iSelection = 0;
	pMenuState->iLeft = left;
	pMenuState->iTop = top;
	pMenuState->iActiveItem = iActiveItem;
	
	// We should draw this thing, probably.
	for (uint8_t i=0; i<nItems; i++)
		MENU_DrawItem (pMenuState, i, i == pMenuState->iSelection, i == pMenuState->iActiveItem);
}

void MENU_Update (MenuState* pMenuState, const InputState* pInputState)
{
	uint8_t selection = pMenuState->iSelection;
	
	if (INPUT_IsButtonPressed (pInputState, Button_Left))
	{
		if (selection == 0)
			selection = pMenuState->nItems;
		selection--;
	} 
	else if (INPUT_IsButtonPressed (pInputState, Button_Right))
	{
		selection++;
		if (selection == pMenuState->nItems)
			selection = 0;
	}
	
	if (selection != pMenuState->iSelection)
	{
		// Remove old selection (and arrow)
		if (pMenuState->iSelection != (uint8_t)-1)
			MENU_DrawItem (pMenuState, pMenuState->iSelection, false, pMenuState->iSelection == pMenuState->iActiveItem);
		
		// Draw new selection.
		pMenuState->iSelection = selection;
		
		MENU_DrawItem (pMenuState, pMenuState->iSelection, true, pMenuState->iSelection == pMenuState->iActiveItem);
		return;
	}
	
	// Handle item activation. Actually, handle several buttons here and let the callback choose what to do.
	static const InputButton handleButtons[] = { Button_Accelerate, Button_Brake, Button_Start };
	for (uint8_t i=0; i<sizeof(handleButtons)/sizeof(InputButton); i++)
	{
		InputButton button = handleButtons[i];
		if (INPUT_IsButtonPressedNoRepeat (pInputState, button))
		{
			const MenuItem* pItem = &pMenuState->pItems[pMenuState->iSelection];
			if (pItem->pCallback (button, pItem->param))
			{
				if (pMenuState->iActiveItem != (uint8_t)-1 &&
					pMenuState->iActiveItem != pMenuState->iSelection)
				{
					MENU_DrawItem (pMenuState, pMenuState->iActiveItem, false, false);
					MENU_DrawItem (pMenuState, pMenuState->iSelection, true, true);
				}
				
				pMenuState->iActiveItem = pMenuState->iSelection; // No need to refresh now, we have the selection.
			}
		}
	}
}
