#ifndef __TEXT_H__
#define __TEXT_H__

#ifndef CPU0
#error "Text RAM can only be accessed on main cpu!"
#endif

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#include <maincpu.h>
#include <stdint.h>
#include <stdbool.h>

// Text RAM is located at 0x110000.
#define TEXT_RAM_BASE             (TILE_RAM_BASE+0x8000)

// The text layer consists of 64x28 tiles, but there are only 40 visible columns starting at offset 24.
#define TEXT_SCREEN_WIDTH          ((uint16_t)64)
#define TEXT_SCREEN_VISIBLE_XSTART ((uint16_t)24)
#define TEXT_SCREEN_VISIBLE_WIDTH  ((uint16_t)40)
#define TEXT_SCREEN_HEIGHT         ((uint16_t)28)

// Defines for our default colors.
typedef uint8_t TEXTCOLOR;
#define TEXT_Red    ((TEXTCOLOR)0)
#define TEXT_Green  ((TEXTCOLOR)1)
#define TEXT_Blue   ((TEXTCOLOR)2)
#define TEXT_Yellow ((TEXTCOLOR)3)
#define TEXT_Purple ((TEXTCOLOR)4)
#define TEXT_Cyan   ((TEXTCOLOR)5)
#define TEXT_Gray   ((TEXTCOLOR)6)
#define TEXT_White  ((TEXTCOLOR)7)

// ROM characters. We don't have the entire ASCII set.
#define TEXT_GLYPH_MASK 0x1ff
typedef uint16_t TEXTGLYPH;
#define TEXTGLYPH_COPYRIGHT   ((TEXTGLYPH)0x10)
#define TEXTGLYPH_EQUAL       ((TEXTGLYPH)0x1b)
#define TEXTGLYPH_CROSS       ((TEXTGLYPH)0x1c)
#define TEXTGLYPH_END         ((TEXTGLYPH)0x3a) // 'ED' for name entry.
#define TEXTGLYPH_ARROWRIGHT  ((TEXTGLYPH)0x3b)
#define TEXTGLYPH_ARROWLEFT   ((TEXTGLYPH)0x3c)
#define TEXTGLYPH_ARROWDOWN   ((TEXTGLYPH)0x3d)
#define TEXTGLYPH_ARROWUP     ((TEXTGLYPH)0x3e)
#define TEXTGLYPH_PERIOD      ((TEXTGLYPH)0x5b)
#define TEXTGLYPH_SINGLEQUOTE ((TEXTGLYPH)0x5e)
#define TEXTGLYPH_DOUBLEQUOTE ((TEXTGLYPH)0x5f)
#define TEXTGLYPH_SOLID_0     ((TEXTGLYPH)0xf6)  // 7 solid blocks. could be used for cursor.
#define TEXTGLYPH_SLASH       ((TEXTGLYPH)0x1ee) // From 'Km/h'

// Fills the entire text buffer with spaces (0x20).
void TEXT_ClearTextRAM ();

// Sets up a palette with the 8 default text colors (as gradients).
void TEXT_InitDefaultPalette ();

// Measure text region on screen.
const char* TEXT_CalcRect (uint16_t* pWidth, uint16_t* pHeight, const char* pText, bool bWrap);

// Draws a rectangle ('window' outline) on the text screen. Does not take the console window setting into account.
// Will simultaneously clear the background when the fill parameter is set to something other than TEXTRECT_NOFILL.
#define TEXTRECT_NOFILL (uint16_t(0xffff))
void TEXT_DrawRect (uint16_t left, uint16_t top, uint16_t right, uint16_t bottom, uint16_t fill /* = TEXTRECT_NOFILL */);

// Defines a specific scroll area on the screen, which can be used as a console.
void TEXT_SetWindow (uint16_t left, uint16_t top, uint16_t right, uint16_t bottom, bool scrollEnable);

// Sets the default text color.
void TEXT_SetColor (TEXTCOLOR colorSetIdx);

// Repositions the cursor, relative to the console window.
void TEXT_GotoXY (uint8_t x, uint8_t y);

// Write raw glyphs, without translation.
void TEXT_WriteRawChar (const TEXTGLYPH c);
void TEXT_WriteRaw (const TEXTGLYPH* glyphs, uint16_t num);

// Convert ASCII character to closest glyph in the original tile ROM.
TEXTGLYPH TEXT_GlyphFromASCII (char c);

// Somewhat ASCII-cmpatible functions.
// Single char.
void TEXT_WriteChar (char c);

// Supports color switching through \001..\010.
void TEXT_Write (const char* text);

// Word-wrapped to current console window.
// Supports color switching through \001..\010.
void TEXT_WriteWrapped (const char* text);

// Write a hexadecimal number, up to 8 digits.
// Padding is prepended if the resulting string is shorter than minWidth.
void TEXT_WriteHex (uint32_t value, uint8_t minWidth, char padding /* = ' ' */);

// Fast hexadecimal number write. No prefix.
void TEXT_WriteHex32 (uint32_t i);
void TEXT_WriteHex8 (uint8_t c);

// Helper to draw a top window with a message, for asserts, fatal errors etc.
// Adjusts the current window and there's no restore functionality yet.
void TEXT_DrawMessageWindow (const char* pText, bool bWordWrap);

#ifdef __cplusplus
} // extern "C"
#endif // __cplusplus

#endif // __TEXT_H__
