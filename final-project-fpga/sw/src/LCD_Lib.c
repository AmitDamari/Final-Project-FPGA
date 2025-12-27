/******************************************************************************
 * LCD_Lib.c
 * 
 * LCD Library - High-level text and basic shape functions
 *
 ******************************************************************************/

#include "LCD_Lib.h"
#include "LCD_Driver.h"
#include "font.h"
#include <stdarg.h>

/*============================================================================
 * Private Variables
 *===========================================================================*/

static uint8_t g_cursor_col = 0;
static uint8_t g_cursor_row = 0;

/*============================================================================
 * Text Drawing Functions
 *===========================================================================*/

void LCD_PutChar(uint8_t x, uint8_t y, char c)
{
    const uint8_t *font_data = Font_GetChar(c);
    if (font_data == NULL) return;
    
    uint8_t page = y / LCD_PAGE_HEIGHT;
    uint8_t bit_offset = y % LCD_PAGE_HEIGHT;
    
    for (uint8_t col = 0; col < FONT_WIDTH; col++) {
        if (x + col >= LCD_WIDTH) break;
        
        uint8_t char_col = font_data[col];
        
        if (bit_offset == 0) {
            /* Aligned to page boundary */
            uint8_t existing = LCD_ReadDataAt(x + col, page);
            LCD_WriteDataAt(x + col, page, existing | char_col);
        } else {
            /* Spans two pages */
            uint8_t upper = char_col << bit_offset;
            uint8_t lower = char_col >> (8 - bit_offset);
            
            if (page < LCD_PAGES) {
                uint8_t existing = LCD_ReadDataAt(x + col, page);
                LCD_WriteDataAt(x + col, page, existing | upper);
            }
            if (page + 1 < LCD_PAGES) {
                uint8_t existing = LCD_ReadDataAt(x + col, page + 1);
                LCD_WriteDataAt(x + col, page + 1, existing | lower);
            }
        }
    }
}

void LCD_PutString(uint8_t x, uint8_t y, const char *str)
{
    while (*str) {
        if (x >= LCD_WIDTH - FONT_WIDTH) break;
        LCD_PutChar(x, y, *str);
        x += CHAR_WIDTH;
        str++;
    }
}

void LCD_Printf(uint8_t x, uint8_t y, const char *fmt, ...)
{
    char buffer[64];
    va_list args;
    
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);
    
    LCD_PutString(x, y, buffer);
}

void LCD_PutStringCentered(uint8_t y, const char *str)
{
    uint16_t width = Font_GetStringWidth(str);
    uint8_t x = (LCD_WIDTH - width) / 2;
    LCD_PutString(x, y, str);
}

void LCD_PutStringRight(uint8_t y, const char *str)
{
    uint16_t width = Font_GetStringWidth(str);
    uint8_t x = LCD_WIDTH - width - 2;
    LCD_PutString(x, y, str);
}

void LCD_PutLargeChar(uint8_t x, uint8_t y, char c)
{
    const uint8_t *font_data = NULL;
    
    if (c >= '0' && c <= '9') {
        font_data = Font_GetLargeDigit(c - '0');
    } else if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z')) {
        font_data = Font_GetLargeLetter(c);
    }
    
    if (font_data == NULL) return;
    
    /* Large font is 8 pixels wide, 14 pixels tall */
    /* Need to handle 2 pages */
    uint8_t start_page = y / LCD_PAGE_HEIGHT;
    uint8_t bit_offset = y % LCD_PAGE_HEIGHT;
    
    for (uint8_t row = 0; row < LARGE_FONT_HEIGHT; row++) {
        uint8_t row_data = font_data[row];
        uint8_t pixel_y = y + row;
        uint8_t page = pixel_y / LCD_PAGE_HEIGHT;
        uint8_t bit = pixel_y % LCD_PAGE_HEIGHT;
        
        if (page >= LCD_PAGES) break;
        
        for (uint8_t col = 0; col < LARGE_FONT_WIDTH; col++) {
            if (x + col >= LCD_WIDTH) break;
            
            if (row_data & (0x80 >> col)) {
                /* Set this pixel */
                uint8_t existing = LCD_ReadDataAt(x + col, page);
                existing |= (1 << bit);
                LCD_WriteDataAt(x + col, page, existing);
            }
        }
    }
}

void LCD_PutLargeString(uint8_t x, uint8_t y, const char *str)
{
    while (*str) {
        if (x >= LCD_WIDTH - LARGE_FONT_WIDTH) break;
        LCD_PutLargeChar(x, y, *str);
        x += LARGE_FONT_WIDTH + 2;  /* 2 pixel spacing */
        str++;
    }
}

/*============================================================================
 * Text Cursor Functions
 *===========================================================================*/

void LCD_SetCursor(uint8_t col, uint8_t row)
{
    if (col >= LCD_CHAR_COLS) col = LCD_CHAR_COLS - 1;
    if (row >= LCD_CHAR_ROWS) row = LCD_CHAR_ROWS - 1;
    
    g_cursor_col = col;
    g_cursor_row = row;
}

uint8_t LCD_GetCursorCol(void)
{
    return g_cursor_col;
}

uint8_t LCD_GetCursorRow(void)
{
    return g_cursor_row;
}

void LCD_WriteChar(char c)
{
    if (c == '\n') {
        g_cursor_col = 0;
        g_cursor_row++;
        if (g_cursor_row >= LCD_CHAR_ROWS) {
            g_cursor_row = 0;
        }
        return;
    }
    
    if (c == '\r') {
        g_cursor_col = 0;
        return;
    }
    
    uint8_t x = g_cursor_col * CHAR_WIDTH;
    uint8_t y = g_cursor_row * CHAR_HEIGHT;
    
    LCD_PutChar(x, y, c);
    
    g_cursor_col++;
    if (g_cursor_col >= LCD_CHAR_COLS) {
        g_cursor_col = 0;
        g_cursor_row++;
        if (g_cursor_row >= LCD_CHAR_ROWS) {
            g_cursor_row = 0;
        }
    }
}

void LCD_WriteString(const char *str)
{
    while (*str) {
        LCD_WriteChar(*str);
        str++;
    }
}

void LCD_ClearLine(void)
{
    uint8_t y = g_cursor_row * CHAR_HEIGHT;
    uint8_t page = y / LCD_PAGE_HEIGHT;
    
    for (uint8_t x = 0; x < LCD_WIDTH; x++) {
        LCD_WriteDataAt(x, page, 0);
    }
    
    g_cursor_col = 0;
}

void LCD_ClearToEOL(void)
{
    uint8_t y = g_cursor_row * CHAR_HEIGHT;
    uint8_t page = y / LCD_PAGE_HEIGHT;
    uint8_t start_x = g_cursor_col * CHAR_WIDTH;
    
    for (uint8_t x = start_x; x < LCD_WIDTH; x++) {
        LCD_WriteDataAt(x, page, 0);
    }
}

/*============================================================================
 * Basic Shape Functions
 *===========================================================================*/

void LCD_SetPixel(uint8_t x, uint8_t y, uint8_t color)
{
    if (x >= LCD_WIDTH || y >= LCD_HEIGHT) return;
    
    uint8_t page = y / LCD_PAGE_HEIGHT;
    uint8_t bit = y % LCD_PAGE_HEIGHT;
    
    uint8_t data = LCD_ReadDataAt(x, page);
    
    if (color) {
        data |= (1 << bit);
    } else {
        data &= ~(1 << bit);
    }
    
    LCD_WriteDataAt(x, page, data);
}

uint8_t LCD_GetPixel(uint8_t x, uint8_t y)
{
    if (x >= LCD_WIDTH || y >= LCD_HEIGHT) return 0;
    
    uint8_t page = y / LCD_PAGE_HEIGHT;
    uint8_t bit = y % LCD_PAGE_HEIGHT;
    
    uint8_t data = LCD_ReadDataAt(x, page);
    
    return (data >> bit) & 1;
}

void LCD_DrawHLine(uint8_t x, uint8_t y, uint8_t len)
{
    for (uint8_t i = 0; i < len && (x + i) < LCD_WIDTH; i++) {
        LCD_SetPixel(x + i, y, 1);
    }
}

void LCD_DrawVLine(uint8_t x, uint8_t y, uint8_t len)
{
    for (uint8_t i = 0; i < len && (y + i) < LCD_HEIGHT; i++) {
        LCD_SetPixel(x, y + i, 1);
    }
}

void LCD_DrawRect(uint8_t x, uint8_t y, uint8_t w, uint8_t h)
{
    LCD_DrawHLine(x, y, w);
    LCD_DrawHLine(x, y + h - 1, w);
    LCD_DrawVLine(x, y, h);
    LCD_DrawVLine(x + w - 1, y, h);
}

void LCD_FillRect(uint8_t x, uint8_t y, uint8_t w, uint8_t h)
{
    for (uint8_t i = 0; i < h; i++) {
        LCD_DrawHLine(x, y + i, w);
    }
}

void LCD_ClearRect(uint8_t x, uint8_t y, uint8_t w, uint8_t h)
{
    for (uint8_t j = 0; j < h && (y + j) < LCD_HEIGHT; j++) {
        for (uint8_t i = 0; i < w && (x + i) < LCD_WIDTH; i++) {
            LCD_SetPixel(x + i, y + j, 0);
        }
    }
}

void LCD_InvertRect(uint8_t x, uint8_t y, uint8_t w, uint8_t h)
{
    for (uint8_t j = 0; j < h && (y + j) < LCD_HEIGHT; j++) {
        for (uint8_t i = 0; i < w && (x + i) < LCD_WIDTH; i++) {
            uint8_t pixel = LCD_GetPixel(x + i, y + j);
            LCD_SetPixel(x + i, y + j, !pixel);
        }
    }
}