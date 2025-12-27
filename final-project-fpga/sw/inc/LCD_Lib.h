/******************************************************************************
 * LCD_Lib.h
 * 
 * LCD Library - High-level text and basic shape functions
 *
 ******************************************************************************/

#ifndef LCD_LIB_H_
#define LCD_LIB_H_

#include "terasic_os_includes.h"
#include "font.h"

/*============================================================================
 * Text Drawing Functions
 *===========================================================================*/

/**
 * @brief Draw a single character
 * @param x X coordinate
 * @param y Y coordinate
 * @param c Character to draw
 */
void LCD_PutChar(uint8_t x, uint8_t y, char c);

/**
 * @brief Draw a string
 * @param x X coordinate
 * @param y Y coordinate
 * @param str String to draw
 */
void LCD_PutString(uint8_t x, uint8_t y, const char *str);

/**
 * @brief Draw a formatted string
 * @param x X coordinate
 * @param y Y coordinate
 * @param fmt Format string (printf-style)
 */
void LCD_Printf(uint8_t x, uint8_t y, const char *fmt, ...);

/**
 * @brief Draw centered string
 * @param y Y coordinate
 * @param str String to draw
 */
void LCD_PutStringCentered(uint8_t y, const char *str);

/**
 * @brief Draw right-aligned string
 * @param y Y coordinate
 * @param str String to draw
 */
void LCD_PutStringRight(uint8_t y, const char *str);

/**
 * @brief Draw large character (for queue display)
 * @param x X coordinate
 * @param y Y coordinate
 * @param c Character ('0'-'9' or 'A'-'Z')
 */
void LCD_PutLargeChar(uint8_t x, uint8_t y, char c);

/**
 * @brief Draw large string
 * @param x X coordinate
 * @param y Y coordinate
 * @param str String to draw
 */
void LCD_PutLargeString(uint8_t x, uint8_t y, const char *str);

/*============================================================================
 * Text Cursor Functions
 *===========================================================================*/

/**
 * @brief Set text cursor position (character coordinates)
 * @param col Column (0-20)
 * @param row Row (0-7)
 */
void LCD_SetCursor(uint8_t col, uint8_t row);

/**
 * @brief Get text cursor column
 * @return Column position
 */
uint8_t LCD_GetCursorCol(void);

/**
 * @brief Get text cursor row
 * @return Row position
 */
uint8_t LCD_GetCursorRow(void);

/**
 * @brief Write character at cursor and advance
 * @param c Character to write
 */
void LCD_WriteChar(char c);

/**
 * @brief Write string at cursor
 * @param str String to write
 */
void LCD_WriteString(const char *str);

/**
 * @brief Clear current line
 */
void LCD_ClearLine(void);

/**
 * @brief Clear from cursor to end of line
 */
void LCD_ClearToEOL(void);

/*============================================================================
 * Basic Shape Functions
 *===========================================================================*/

/**
 * @brief Set a single pixel
 * @param x X coordinate
 * @param y Y coordinate
 * @param color 0=off, 1=on
 */
void LCD_SetPixel(uint8_t x, uint8_t y, uint8_t color);

/**
 * @brief Get pixel value
 * @param x X coordinate
 * @param y Y coordinate
 * @return 0=off, 1=on
 */
uint8_t LCD_GetPixel(uint8_t x, uint8_t y);

/**
 * @brief Draw horizontal line
 * @param x Start X coordinate
 * @param y Y coordinate
 * @param len Length in pixels
 */
void LCD_DrawHLine(uint8_t x, uint8_t y, uint8_t len);

/**
 * @brief Draw vertical line
 * @param x X coordinate
 * @param y Start Y coordinate
 * @param len Length in pixels
 */
void LCD_DrawVLine(uint8_t x, uint8_t y, uint8_t len);

/**
 * @brief Draw rectangle outline
 * @param x X coordinate
 * @param y Y coordinate
 * @param w Width
 * @param h Height
 */
void LCD_DrawRect(uint8_t x, uint8_t y, uint8_t w, uint8_t h);

/**
 * @brief Draw filled rectangle
 * @param x X coordinate
 * @param y Y coordinate
 * @param w Width
 * @param h Height
 */
void LCD_FillRect(uint8_t x, uint8_t y, uint8_t w, uint8_t h);

/**
 * @brief Clear rectangle area
 * @param x X coordinate
 * @param y Y coordinate
 * @param w Width
 * @param h Height
 */
void LCD_ClearRect(uint8_t x, uint8_t y, uint8_t w, uint8_t h);

/**
 * @brief Invert rectangle area
 * @param x X coordinate
 * @param y Y coordinate
 * @param w Width
 * @param h Height
 */
void LCD_InvertRect(uint8_t x, uint8_t y, uint8_t w, uint8_t h);

#endif /* LCD_LIB_H_ */