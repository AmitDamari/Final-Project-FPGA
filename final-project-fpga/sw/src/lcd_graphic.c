/******************************************************************************
 * lcd_graphic.c
 * 
 * LCD Graphics Library - Advanced drawing functions
 *
 ******************************************************************************/

#include "lcd_graphic.h"
#include "LCD_Driver.h"
#include "LCD_Lib.h"
#include "font.h"

/*============================================================================
 * Private Variables
 *===========================================================================*/

static DrawMode_t g_draw_mode = DRAW_MODE_NORMAL;

/*============================================================================
 * Drawing Mode Functions
 *===========================================================================*/

void DRAW_SetMode(DrawMode_t mode)
{
    g_draw_mode = mode;
}

DrawMode_t DRAW_GetMode(void)
{
    return g_draw_mode;
}

/*============================================================================
 * Basic Drawing Functions
 *===========================================================================*/

void DRAW_Pixel(uint8_t x, uint8_t y, uint8_t color)
{
    if (x >= LCD_WIDTH || y >= LCD_HEIGHT) return;
    
    uint8_t page = y / LCD_PAGE_HEIGHT;
    uint8_t bit = y % LCD_PAGE_HEIGHT;
    uint8_t mask = 1 << bit;
    
    uint8_t data = LCD_ReadDataAt(x, page);
    
    switch (g_draw_mode) {
        case DRAW_MODE_NORMAL:
            if (color) {
                data |= mask;
            } else {
                data &= ~mask;
            }
            break;
            
        case DRAW_MODE_XOR:
            if (color) {
                data ^= mask;
            }
            break;
            
        case DRAW_MODE_CLEAR:
            data &= ~mask;
            break;
            
        case DRAW_MODE_INVERT:
            data ^= mask;
            break;
    }
    
    LCD_WriteDataAt(x, page, data);
}

void DRAW_Line(int x0, int y0, int x1, int y1, uint8_t color)
{
    /* Bresenham's line algorithm */
    int dx = abs(x1 - x0);
    int dy = abs(y1 - y0);
    int sx = (x0 < x1) ? 1 : -1;
    int sy = (y0 < y1) ? 1 : -1;
    int err = dx - dy;
    
    while (1) {
        if (x0 >= 0 && x0 < LCD_WIDTH && y0 >= 0 && y0 < LCD_HEIGHT) {
            DRAW_Pixel(x0, y0, color);
        }
        
        if (x0 == x1 && y0 == y1) break;
        
        int e2 = 2 * err;
        if (e2 > -dy) {
            err -= dy;
            x0 += sx;
        }
        if (e2 < dx) {
            err += dx;
            y0 += sy;
        }
    }
}

void DRAW_Rect(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, uint8_t color)
{
    /* Ensure x1 < x2 and y1 < y2 */
    if (x1 > x2) { uint8_t t = x1; x1 = x2; x2 = t; }
    if (y1 > y2) { uint8_t t = y1; y1 = y2; y2 = t; }
    
    /* Top line */
    for (uint8_t x = x1; x <= x2 && x < LCD_WIDTH; x++) {
        DRAW_Pixel(x, y1, color);
    }
    /* Bottom line */
    for (uint8_t x = x1; x <= x2 && x < LCD_WIDTH; x++) {
        DRAW_Pixel(x, y2, color);
    }
    /* Left line */
    for (uint8_t y = y1; y <= y2 && y < LCD_HEIGHT; y++) {
        DRAW_Pixel(x1, y, color);
    }
    /* Right line */
    for (uint8_t y = y1; y <= y2 && y < LCD_HEIGHT; y++) {
        DRAW_Pixel(x2, y, color);
    }
}

void DRAW_RectFill(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, uint8_t color)
{
    if (x1 > x2) { uint8_t t = x1; x1 = x2; x2 = t; }
    if (y1 > y2) { uint8_t t = y1; y1 = y2; y2 = t; }
    
    for (uint8_t y = y1; y <= y2 && y < LCD_HEIGHT; y++) {
        for (uint8_t x = x1; x <= x2 && x < LCD_WIDTH; x++) {
            DRAW_Pixel(x, y, color);
        }
    }
}

void DRAW_Circle(int cx, int cy, int radius, uint8_t color)
{
    /* Midpoint circle algorithm */
    int x = radius;
    int y = 0;
    int err = 0;
    
    while (x >= y) {
        DRAW_Pixel(cx + x, cy + y, color);
        DRAW_Pixel(cx + y, cy + x, color);
        DRAW_Pixel(cx - y, cy + x, color);
        DRAW_Pixel(cx - x, cy + y, color);
        DRAW_Pixel(cx - x, cy - y, color);
        DRAW_Pixel(cx - y, cy - x, color);
        DRAW_Pixel(cx + y, cy - x, color);
        DRAW_Pixel(cx + x, cy - y, color);
        
        y++;
        if (err <= 0) {
            err += 2 * y + 1;
        }
        if (err > 0) {
            x--;
            err -= 2 * x + 1;
        }
    }
}

void DRAW_CircleFill(int cx, int cy, int radius, uint8_t color)
{
    for (int y = -radius; y <= radius; y++) {
        for (int x = -radius; x <= radius; x++) {
            if (x*x + y*y <= radius*radius) {
                DRAW_Pixel(cx + x, cy + y, color);
            }
        }
    }
}

void DRAW_RoundRect(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2,
                    uint8_t radius, uint8_t color)
{
    if (x1 > x2) { uint8_t t = x1; x1 = x2; x2 = t; }
    if (y1 > y2) { uint8_t t = y1; y1 = y2; y2 = t; }
    
    /* Draw straight edges */
    for (uint8_t x = x1 + radius; x <= x2 - radius; x++) {
        DRAW_Pixel(x, y1, color);
        DRAW_Pixel(x, y2, color);
    }
    for (uint8_t y = y1 + radius; y <= y2 - radius; y++) {
        DRAW_Pixel(x1, y, color);
        DRAW_Pixel(x2, y, color);
    }
    
    /* Draw corners using circle algorithm */
    int r = radius;
    int x = r;
    int y = 0;
    int err = 0;
    
    while (x >= y) {
        /* Top-left corner */
        DRAW_Pixel(x1 + r - x, y1 + r - y, color);
        DRAW_Pixel(x1 + r - y, y1 + r - x, color);
        /* Top-right corner */
        DRAW_Pixel(x2 - r + x, y1 + r - y, color);
        DRAW_Pixel(x2 - r + y, y1 + r - x, color);
        /* Bottom-left corner */
        DRAW_Pixel(x1 + r - x, y2 - r + y, color);
        DRAW_Pixel(x1 + r - y, y2 - r + x, color);
        /* Bottom-right corner */
        DRAW_Pixel(x2 - r + x, y2 - r + y, color);
        DRAW_Pixel(x2 - r + y, y2 - r + x, color);
        
        y++;
        if (err <= 0) {
            err += 2 * y + 1;
        }
        if (err > 0) {
            x--;
            err -= 2 * x + 1;
        }
    }
}

void DRAW_RoundRectFill(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2,
                        uint8_t radius, uint8_t color)
{
    if (x1 > x2) { uint8_t t = x1; x1 = x2; x2 = t; }
    if (y1 > y2) { uint8_t t = y1; y1 = y2; y2 = t; }
    
    /* Fill main rectangle */
    DRAW_RectFill(x1 + radius, y1, x2 - radius, y2, color);
    DRAW_RectFill(x1, y1 + radius, x2, y2 - radius, color);
    
    /* Fill corners */
    int r = radius;
    for (int dy = 0; dy <= r; dy++) {
        for (int dx = 0; dx <= r; dx++) {
            if (dx*dx + dy*dy <= r*r) {
                DRAW_Pixel(x1 + r - dx, y1 + r - dy, color);
                DRAW_Pixel(x2 - r + dx, y1 + r - dy, color);
                DRAW_Pixel(x1 + r - dx, y2 - r + dy, color);
                DRAW_Pixel(x2 - r + dx, y2 - r + dy, color);
            }
        }
    }
}

/*============================================================================
 * Text Drawing Functions
 *===========================================================================*/

void DRAW_Char(uint8_t x, uint8_t y, char c)
{
    LCD_PutChar(x, y, c);
}

void DRAW_String(uint8_t x, uint8_t y, const char *str)
{
    LCD_PutString(x, y, str);
}

void DRAW_StringInvert(uint8_t x, uint8_t y, const char *str)
{
    uint16_t width = Font_GetStringWidth(str);
    
    /* Draw background */
    DRAW_RectFill(x, y, x + width, y + FONT_HEIGHT, 1);
    
    /* Draw text in XOR mode */
    DrawMode_t old_mode = g_draw_mode;
    g_draw_mode = DRAW_MODE_XOR;
    
    while (*str) {
        const uint8_t *font_data = Font_GetChar(*str);
        if (font_data != NULL) {
            uint8_t page = y / LCD_PAGE_HEIGHT;
            for (uint8_t col = 0; col < FONT_WIDTH; col++) {
                if (x + col >= LCD_WIDTH) break;
                uint8_t existing = LCD_ReadDataAt(x + col, page);
                LCD_WriteDataAt(x + col, page, existing ^ font_data[col]);
            }
        }
        x += CHAR_WIDTH;
        str++;
    }
    
    g_draw_mode = old_mode;
}

void DRAW_StringCenter(uint8_t y, const char *str)
{
    LCD_PutStringCentered(y, str);
}

/*============================================================================
 * Bitmap Functions
 *===========================================================================*/

void DRAW_Bitmap(uint8_t x, uint8_t y, uint8_t width, uint8_t height,
                 const uint8_t *bitmap)
{
    if (bitmap == NULL) return;
    
    uint8_t bytes_per_row = (width + 7) / 8;
    
    for (uint8_t row = 0; row < height; row++) {
        if (y + row >= LCD_HEIGHT) break;
        
        for (uint8_t col = 0; col < width; col++) {
            if (x + col >= LCD_WIDTH) break;
            
            uint8_t byte_idx = row * bytes_per_row + (col / 8);
            uint8_t bit_idx = 7 - (col % 8);
            
            if (bitmap[byte_idx] & (1 << bit_idx)) {
                DRAW_Pixel(x + col, y + row, 1);
            }
        }
    }
}

void DRAW_BitmapTransparent(uint8_t x, uint8_t y, uint8_t width, uint8_t height,
                            const uint8_t *bitmap)
{
    if (bitmap == NULL) return;
    
    uint8_t bytes_per_row = (width + 7) / 8;
    
    for (uint8_t row = 0; row < height; row++) {
        if (y + row >= LCD_HEIGHT) break;
        
        for (uint8_t col = 0; col < width; col++) {
            if (x + col >= LCD_WIDTH) break;
            
            uint8_t byte_idx = row * bytes_per_row + (col / 8);
            uint8_t bit_idx = 7 - (col % 8);
            
            if (bitmap[byte_idx] & (1 << bit_idx)) {
                DRAW_Pixel(x + col, y + row, 1);
            }
            /* Don't clear pixels for transparent bitmaps */
        }
    }
}

/*============================================================================
 * Progress Bar / Meter Functions
 *===========================================================================*/

void DRAW_ProgressBar(uint8_t x, uint8_t y, uint8_t width, uint8_t height,
                      uint8_t percent)
{
    if (percent > 100) percent = 100;
    
    /* Draw border */
    DRAW_Rect(x, y, x + width - 1, y + height - 1, 1);
    
    /* Calculate fill width */
    uint8_t fill_width = ((width - 4) * percent) / 100;
    
    /* Fill the bar */
    if (fill_width > 0) {
        DRAW_RectFill(x + 2, y + 2, x + 2 + fill_width - 1, y + height - 3, 1);
    }
}

void DRAW_VerticalBar(uint8_t x, uint8_t y_base, uint8_t width,
                      uint8_t max_height, uint8_t value, uint8_t max_value)
{
    if (max_value == 0) return;
    
    uint8_t bar_height = (max_height * value) / max_value;
    if (bar_height == 0 && value > 0) bar_height = 1;
    if (bar_height > max_height) bar_height = max_height;
    
    uint8_t y_top = y_base - bar_height + 1;
    
    DRAW_RectFill(x, y_top, x + width - 1, y_base, 1);
}

/*============================================================================
 * Screen Effects
 *===========================================================================*/

void DRAW_ScreenFrame(void)
{
    DRAW_Rect(0, 0, LCD_WIDTH - 1, LCD_HEIGHT - 1, 1);
}

void DRAW_HeaderBar(const char *title)
{
    /* Draw filled header bar */
    DRAW_RectFill(0, 0, LCD_WIDTH - 1, 9, 1);
    
    /* Draw title text (inverted) */
    if (title != NULL) {
        uint16_t title_width = Font_GetStringWidth(title);
        uint8_t x = (LCD_WIDTH - title_width) / 2;
        
        /* Draw in XOR mode for inverted text */
        DrawMode_t old_mode = g_draw_mode;
        g_draw_mode = DRAW_MODE_XOR;
        LCD_PutString(x, 1, title);
        g_draw_mode = old_mode;
    }
}

void DRAW_FooterBar(const char *text)
{
    uint8_t y = LCD_HEIGHT - 10;
    
    /* Draw separator line */
    for (uint8_t x = 0; x < LCD_WIDTH; x++) {
        DRAW_Pixel(x, y, 1);
    }
    
    /* Draw text */
    if (text != NULL) {
        LCD_PutString(2, y + 2, text);
    }
}

void DRAW_DialogBox(const char *title, const char *message)
{
    uint8_t box_x = 10;
    uint8_t box_y = 15;
    uint8_t box_w = LCD_WIDTH - 20;
    uint8_t box_h = 35;
    
    /* Clear area */
    DRAW_RectFill(box_x, box_y, box_x + box_w - 1, box_y + box_h - 1, 0);
    
    /* Draw double border */
    DRAW_Rect(box_x, box_y, box_x + box_w - 1, box_y + box_h - 1, 1);
    DRAW_Rect(box_x + 2, box_y + 2, box_x + box_w - 3, box_y + box_h - 3, 1);
    
    /* Draw title */
    if (title != NULL) {
        uint16_t title_width = Font_GetStringWidth(title);
        uint8_t title_x = box_x + (box_w - title_width) / 2;
        LCD_PutString(title_x, box_y + 5, title);
        
        /* Draw separator line */
        for (uint8_t x = box_x + 5; x < box_x + box_w - 5; x++) {
            DRAW_Pixel(x, box_y + 13, 1);
        }
    }
    
    /* Draw message */
    if (message != NULL) {
        uint16_t msg_width = Font_GetStringWidth(message);
        uint8_t msg_x = box_x + (box_w - msg_width) / 2;
        uint8_t msg_y = title ? box_y + 18 : box_y + 12;
        LCD_PutString(msg_x, msg_y, message);
    }
}