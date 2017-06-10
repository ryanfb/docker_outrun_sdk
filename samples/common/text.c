#include "text.h"
#include "palette.h"
#include <stdbool.h>
#include <stddef.h>

void TEXT_InitDefaultPalette ()
{
	// Sets up 8 different colors sets. We don't have black, because it's ugly.
	uint16_t offset;
	for (offset=1; offset<7; offset++)
	{
		uint16_t gradient = (offset*9)+9;
		uint16_t high = gradient;
		uint16_t low = gradient >> 1;
		if (high > 31)
			high = 31;

		PALETTE_SetColorRGB( 0+offset, high, low,  low) ; // Red
		PALETTE_SetColorRGB( 8+offset, low,  high, low) ; // Green
		PALETTE_SetColorRGB(16+offset, low,  low,  high); // Blue

		PALETTE_SetColorRGB(24+offset, high, high, low ); // Yellow
		PALETTE_SetColorRGB(32+offset, high, low,  high); // Purple
		PALETTE_SetColorRGB(40+offset, low,  high, high); // Cyan
	
		PALETTE_SetColorRGB(48+offset, low,  low,  low ); // Gray
		PALETTE_SetColorRGB(56+offset, high, high, high); // White
	}

	// Background and outline.
	for (offset=0; offset<64; offset+=8)
	{
		PALETTE_SetColor16 (offset,   0); // Background.
		PALETTE_SetColor16 (offset+7, 0); // Outline.
	}
}

void TEXT_ClearTextRAM ()
{
	// Clear 0x000 through 0xDFE with 0x0020.
	uint16_t offset;
	for (offset=0; offset<0x700; offset++)
		TEXT_RAM_BASE[offset] = 0x0020;
}

static uint16_t textWindowLeft   = TEXT_SCREEN_WIDTH-TEXT_SCREEN_VISIBLE_WIDTH;
static uint16_t textWindowTop    = 0;
static uint16_t textWindowRight  = TEXT_SCREEN_WIDTH;
static uint16_t textWindowBottom = TEXT_SCREEN_HEIGHT;
static uint16_t textColorMask    = 0x8000 | (TEXT_White << 9);
static uint16_t textCursorX      = TEXT_SCREEN_WIDTH-TEXT_SCREEN_VISIBLE_WIDTH;
static uint16_t textCursorY      = 0;
static bool     scrollEnabled    = true;

void TEXT_SetWindow (uint16_t left, uint16_t top, uint16_t right, uint16_t bottom, bool enableScroll)
{
	// Sanity checks.
	if (right <= left || bottom <= top)
		return;
	if (right > TEXT_SCREEN_WIDTH)
		right = TEXT_SCREEN_WIDTH;
	if (bottom > TEXT_SCREEN_HEIGHT)
		bottom = TEXT_SCREEN_HEIGHT;

	textWindowLeft   = left;
	textWindowTop    = top;
	textWindowRight  = right;
	textWindowBottom = bottom;
	
	// Enable/disable scrolling on this viewport.
	scrollEnabled = enableScroll;

	// Set the cursor at the top left.
	textCursorX = left;
	textCursorY = top;
}

void TEXT_SetColor (uint8_t colorSetIdx)
{
	colorSetIdx &= 0x7;
	textColorMask = (colorSetIdx << 9) | 0x8000; // Give text priority against sprites.
}

void TEXT_GotoXY (uint8_t x, uint8_t y)
{
	textCursorX = x + textWindowLeft;
	textCursorY = y + textWindowTop;
}

static void TEXT_ScrollWindow ()
{
	uint16_t x, y;
	uint16_t* pDest = TEXT_RAM_BASE + textWindowTop * TEXT_SCREEN_WIDTH;
	const uint16_t* pSrc = pDest + TEXT_SCREEN_WIDTH;
	for (y=textWindowTop; y<(textWindowBottom-1); y++)
	{
		for (x=textWindowLeft; x<textWindowRight; x++)
			pDest[x] = pSrc[x];
		pDest += TEXT_SCREEN_WIDTH;
		pSrc  += TEXT_SCREEN_WIDTH;
	}

	// Clear the bottom row.
	for (x=textWindowLeft; x<textWindowRight; x++)
		pDest[x] = 0x0020;
}

void TEXT_WriteRawChar (const TEXTGLYPH c)
{
	// Write out character.
	TEXT_RAM_BASE[textCursorY*TEXT_SCREEN_WIDTH+textCursorX] = (c & TEXT_GLYPH_MASK) | textColorMask;
	
	// Update screen position.
	textCursorX++;
	if (textCursorX == textWindowRight)
	{
		textCursorX = textWindowLeft;
		textCursorY++;
		if (textCursorY == textWindowBottom)
		{
			if (scrollEnabled)
				TEXT_ScrollWindow();
			textCursorY--;
		}
	}
}

void TEXT_WriteRaw (const TEXTGLYPH* glyphs, uint16_t num) 
{
	while(num--)
		TEXT_WriteRawChar (*glyphs++);
}

// Convert ASCII character to closest glyph in the original tile ROM.
TEXTGLYPH TEXT_GlyphFromASCII (char c)
{
	switch (c)
	{
	case 'A'...'Z':
	case '0'...'9':
		return c;
		
	case 'a'...'z':
		return c = 'A' + (c-'a');
	
	case ' ':
	case '!':
	case '#':
	case '%':
	case '&':
	case '*':
	case '+':
	case '-':
		return (TEXTGLYPH)c;

	case '=':
		return TEXTGLYPH_EQUAL;
	case '.':
		return TEXTGLYPH_PERIOD;
	case '/':
		return TEXTGLYPH_SLASH;
	case '\'':
		return TEXTGLYPH_SINGLEQUOTE;
	case '\"':
		return TEXTGLYPH_DOUBLEQUOTE;
	case '_':
		return 0xe; // Part of an outline, which paints the lower 2 pixels over the entire width.
		
	default:
		return TEXTGLYPH_CROSS;
	}
}

void TEXT_WriteChar (char c)
{
	switch (c)
	{
	case '\n':
		{
			textCursorX = textWindowLeft;
			textCursorY++;
			if (textCursorY == textWindowBottom)
			{
				if (scrollEnabled)
					TEXT_ScrollWindow();
				textCursorY--;
			}
		}
		break;
	
	case '\0':
		return;
		
	default:
		{
			TEXTGLYPH glyph = TEXT_GlyphFromASCII(c);
			TEXT_WriteRawChar (glyph);
		}
	}
}

void TEXT_Write (const char* text)
{
	const char* textPtr = text;
	char c;
	while ((c = *textPtr++))
	{
		if (c <= 8)
			TEXT_SetColor(c-1);
		else
			TEXT_WriteChar (c);
	}
}

void TEXT_WriteWrapped (const char* text)
{
	// Keep walking the current word, and then fit it to the screen.
	const char* curStart = text;
	const char* curEnd = text;
	const uint16_t windowWidth = textWindowRight - textWindowLeft;
	uint16_t wordLength = 0;
	
	for (;;)
	{
		// Inspect the byte at our current position.
		char c = *curEnd;
		switch (c)
		{
		case 1 ... 8:
			// If there's no word buffered, process colors immediatly.
			// Otherwise they're part of the word.
			if (!wordLength)
			{
				TEXT_SetColor(c-1);
				curStart = ++curEnd;
			}
			else ++curEnd; // Add it to the word (but don't count as length).
			break;
			
		case '\0':
		case ' ':
		case '\n':
			{
				bool stealWhitespace = false;
				if (curEnd > curStart)
				{
					// There was a buffered word. Fit it to the current line.
					uint16_t lineRemaining = textWindowRight - textCursorX;
					if (wordLength <= lineRemaining)
					{
						// Special case: if we got onto a new line PRECISELY, ignore the space or newline.
						stealWhitespace = (wordLength == lineRemaining);
					}
					else if (wordLength <= windowWidth)
					{
						TEXT_WriteChar ('\n'); // Move to a new line first.
					}
					else
					{
						// It won't fit on the new line either. Just proceed as if we had no wrapping (also: do not wrap for the start).
					}
					
					// Write the buffered word.
					while (curStart != curEnd)
					{
						char c = *curStart++;
						if (c <= 8)
							TEXT_SetColor(c-1);
						else
							TEXT_WriteChar (c);
					}
				}
					
				// Done?
				if (!c)
					return;
				
				if (!stealWhitespace)
					TEXT_WriteChar (c);
				
				// Advance.
				curStart = ++curEnd;
				wordLength = 0;
			}
			break;
			
		default:
			// Add to the current word.
			++curEnd;
			++wordLength;
			break;
		}
	}
}

const char* TEXT_CalcRect (uint16_t* pWidth, uint16_t* pHeight, const char* pText, bool bWrap)
{
	const uint16_t maxWidth = *pWidth;
	const uint16_t maxHeight = *pHeight;
	uint16_t minWidth = 0; // Largest line width.
	uint16_t x = 0, y = 0;
	
	if (!maxWidth || !maxHeight)
		return NULL; // Can't fit jack shit.
	
	if (!bWrap)
	{
		bool done = false;
		while (!done)
		{
			char c = *pText;
			switch (c)
			{
			case 0:
				done = true;
				break; // All done.
			case 1 ... 8:
				++pText; // Skip over color markup.
				break;
			case '\n':
				x=0; ++y; // Newline.
				if (y == maxHeight)
					done = true; // Filled up our entire window.
				else
					++pText;
				break;
				
			default:
				// Got a text character here.
				// Assume we're still inside our window.
				++x; // So we can do this.
				if (x > minWidth)
					minWidth = x;
				
				if (x == maxWidth) // If we're at the end, we shall now move on to a new line.
				{
					x = 0;
					++y;
					if (y == maxHeight)
						done = true;
				}
				
				if (!done)
					++pText;
			} // switch (c)
		} // for (;;)
	}
	else
	{
		// Keep walking the current word, and then fit it to the screen.
		const char* curStart = pText;
		const char* curEnd = pText;
		const uint16_t windowWidth = maxWidth;
		uint16_t wordLength = 0;
		bool done = false;
	
		while (!done)
		{
			// Inspect the byte at our current position.
			char c = *curEnd;
			switch (c)
			{
			case 1 ... 8:
				// If there's no word buffered, process colors immediatly.
				// Otherwise they're part of the word.
				if (!wordLength)
					curStart = ++curEnd;
				else 
					++curEnd; // Add it to the word (but don't count as length).
				break;
				
			case '\0':
			case ' ':
			case '\n':
				{
					bool stealWhitespace = false;
					if (curEnd > curStart)
					{
						// There was a buffered word. Fit it to the current line.
						uint16_t lineRemaining = windowWidth - x;
						if (wordLength <= lineRemaining)
						{
							// Special case: if we got onto a new line PRECISELY, ignore the space or newline.
							stealWhitespace = (wordLength == lineRemaining);
						}
						else if (wordLength <= windowWidth)
						{
							// Can fit on the next line.
							x=0; ++y;
							if (y == maxHeight)
							{
								// The new word won't fit. Screen is full.
								// pHeight is correct.
								*pWidth = minWidth;
								return curStart;
							}
						}
						else
						{
							// It won't fit on the new line either. Just proceed as if we had no wrapping (also: do not wrap for the start).
						}
						
						// Write the buffered word.
						while (curStart != curEnd)
						{
							char c = *curStart++;
							if (c > 8)
							{
								// This would yield a character. Handle newlines and the likes here.
								++x;
								if (x > minWidth)
									minWidth = x;
								
								if (x == maxWidth)
								{
									// Move on to new line, if we can.
									x=0;
									++y;
									if (y == maxHeight)
									{
										// Wrapped over the edge on the last line.
										// pHeight is correct.
										*pWidth = minWidth;
										return curStart;
									}
								}
							}
						}
					}
						
					// Done?
					if (!c)
					{
						pText = curEnd; // For the return value.
						done = true;
						break;
					}
					
					if (!stealWhitespace)
					{
						if (c == '\n')
						{
							x=0; ++y;
							if (y == maxHeight)
							{
								// Wrapped over the edge on the last line.
								// pHeight is correct.
								*pWidth = minWidth;
								return curEnd;
							}
						}
						else
						{
							++x;
							if (x > minWidth && !wordLength) // Don't count spaces as last character.
								minWidth = x;
							
							if (x == maxWidth)
							{
								x=0;
								++y;
								if (y == maxHeight)
								{
									*pWidth = minWidth;
									return curEnd;
								}
							}
						}
					}
					
					// Advance.
					curStart = ++curEnd;
					wordLength = 0;
				}
				break;
				
			default:
				// Add to the current word.
				++curEnd;
				++wordLength;
				break;
			}
		}	
	}

	// Copy output values.
	*pWidth = minWidth;
	
	// Note: This won't match the TEXT_Write function exactly, because it'd scroll regardless of whether something comes next.
	// Best to disable scrolling when drawing measured text (if it fits onto the screen).
	if (x > 0)
		*pHeight = y+1; // Current line is in use.
	else
		*pHeight = y; // Exclude current line.
	
	// We ended up here in our string.
	return pText;
}

void TEXT_DrawRect (uint16_t left, uint16_t top, uint16_t right, uint16_t bottom, uint16_t fill /* = 0xffff */)
{
	if (right  >= (left+2) &&
		bottom >= (top+2))
	{
		uint16_t colorMask = 0x8000;
		uint16_t x, y;

		uint16_t* offset = TEXT_RAM_BASE+top*TEXT_SCREEN_WIDTH;
		offset[left] = colorMask | 1;
		for (x=left+1; x<right-1; x++)
			offset[x] = colorMask | 2;
		offset[right-1] = colorMask | 3;
		offset += TEXT_SCREEN_WIDTH;

		for (y=top+1; y<bottom-1; y++)
		{
			offset[left] = colorMask | 4;
			
			// Clear background (optional).
			if (fill != 0xffff)
				for (x=left+1; x<right-1; x++)
					offset[x] = fill;
					
			offset[right-1] = colorMask | 5;
			offset += TEXT_SCREEN_WIDTH;
		}

		for (x=left+1; x<right-1; x++)
		{
			offset[left] = colorMask | 6;
			for (x=left+1; x<right-1; x++)
				offset[x] = colorMask | 7;
			offset[right-1] = colorMask | 8;
		}
	}
}

void TEXT_DrawMessageWindow (const char* pText, bool bWordWrap)
{
	uint16_t rectWidth = TEXT_SCREEN_VISIBLE_WIDTH-2, rectHeight = TEXT_SCREEN_HEIGHT-2;
	TEXT_CalcRect (&rectWidth, &rectHeight, pText, bWordWrap);
	
	uint16_t textX = TEXT_SCREEN_VISIBLE_XSTART + ((TEXT_SCREEN_VISIBLE_WIDTH - rectWidth) >> 1);
	uint16_t textY = ((TEXT_SCREEN_HEIGHT - rectHeight) >> 1);
	TEXT_DrawRect (textX-1, textY-1, textX+rectWidth+1, textY+rectHeight+1, ' ');
	
	TEXT_SetWindow (textX, textY, textX+rectWidth, textY+rectHeight, false);
	
	if (bWordWrap)
		TEXT_WriteWrapped (pText);
	else 
		TEXT_Write (pText);
}

static const char hexTable[] = "0123456789ABCDEF";

// Write a hexadecimal number, up to 8 digits.
// Padding is prepended if the resulting string is shorter than minWidth.
void TEXT_WriteHex (uint32_t value, uint8_t minWidth, char padding /* = ' ' */)
{
	if (minWidth > 8)
		minWidth = 8;
	
	char hexString[9]; // Should be large enough for minWidth!
	const uint8_t maxChars = sizeof(hexString)-1;
	uint8_t writePos = maxChars;
	hexString[writePos] = '\0';
	
	while (value != 0)
	{
		uint8_t digit = value & 0xf;
		value >>= 4;
		hexString[--writePos] = hexTable[digit];
	}
	
	// Add at least one zero.
	if (writePos == maxChars)
		hexString[--writePos] = '0';
	
	while (maxChars-writePos < minWidth)
		hexString[--writePos] = padding;
	
	TEXT_Write (hexString + writePos);
}

void TEXT_WriteHex32 (uint32_t i)
{
	uint32_t shift = 32;
	do
	{
		shift -= 4;
		TEXT_WriteChar (hexTable[(i >> shift) & 0xf]);
	} while (shift);
}

void TEXT_WriteHex8 (uint8_t c)
{
	TEXT_WriteChar (hexTable[(c >> 4)  & 0xf]);
	TEXT_WriteChar (hexTable[c & 0xf]);
}
