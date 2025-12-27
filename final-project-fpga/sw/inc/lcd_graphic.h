/******************************************************************************
 * lcd_graphic.h
 * 
 * LCD Graphics Library - Advanced drawing functions
 *
 ******************************************************************************/

#ifndef LCD_GRAPHIC_H_
#define LCD_GRAPHIC_H_

#include "terasic_os_includes.h"

/*============================================================================
 * Drawing Mode
 *===========================================================================*/

typedef enum {
    DRAW_MODE_NORMAL = 0,   /* Normal drawing (pixel on) */
    DRAW_MODE_XOR,          /* XOR with existing pixels */
    DRAW_MODE_CLEAR,        /* Clear pixels */
    DRAW_MODE_INVERT        /* Invert pixels */
} DrawMode_t;

/**
 * @brief Set drawing mode for subsequent operations
 * @param mode Drawing mode
 */
void DRAW_SetMode(DrawMode_t mode);

/**
 * @brief Get current drawing mode
 * @return Current drawing mode
 */
DrawMode_t DRAW_GetMode(void);

/*============================================================================
 * Basic Drawing Functions
 *===========================================================================*/

/**
 * @brief Draw a single pixel
 * @param x X coordinate
 * @param y Y coordinate
 * @param color 0=off, 1=on, 2=toggle
 */
void DRAW_Pixel(uint8_t x, uint8_t y, uint8_t color);

/**
 * @brief Draw a line
 * @param x0 Start X
 * @param y0 Start Y
 * @param x1 End X
 * @param y1 End Y
 * @param color Color (0/1)
 */
void DRAW_Line(int x0, int y0, int x1, int y1, uint8_t color);

/**
 * @brief Draw a rectangle outline
 * @param x1 Top-left X
 * @param y1 Top-left Y
 * @param x2 Bottom-right X
 * @param y2 Bottom-right Y
 * @param color Color (0/1)
 */
void DRAW_Rect(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, uint8_t color);

/**
 * @brief Draw a filled rectangle
 * @param x1 Top-left X
 * @param y1 Top-left Y
 * @param x2 Bottom-right X
 * @param y2 Bottom-right Y
 * @param color Color (0/1)
 */
void DRAW_RectFill(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, uint8_t color);

/**
 * @brief Draw a circle outline
 * @param cx Center X
 * @param cy Center Y
 * @param radius Radius
 * @param color Color (0/1)
 */
void DRAW_Circle(int cx, int cy, int radius, uint8_t color);

/**
 * @brief Draw a filled circle
 * @param cx Center X
 * @param cy Center Y
 * @param radius Radius
 * @param color Color (0/1)
 */
void DRAW_CircleFill(int cx, int cy, int radius, uint8_t color);

/**
 * @brief Draw rounded rectangle outline
 * @param x1 Top-left X
 * @param y1 Top-left Y
 * @param x2 Bottom-right X
 * @param y2 Bottom-right Y
 * @param radius Corner radius
 * @param color Color (0/1)
 */
void DRAW_RoundRect(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, 
                    uint8_t radius, uint8_t color);

/**
 * @brief Draw filled rounded rectangle
 * @param x1 Top-left X
 * @param y1 Top-left Y
 * @param x2 Bottom-right X
 * @param y2 Bottom-right Y
 * @param radius Corner radius
 * @param color Color (0/1)
 */
void DRAW_RoundRectFill(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2,
                        uint8_t radius, uint8_t color);

/*============================================================================
 * Text Drawing Functions
 *===========================================================================*/

/**
 * @brief Draw a character
 * @param x X coordinate
 * @param y Y coordinate
 * @param c Character
 */
void DRAW_Char(uint8_t x, uint8_t y, char c);

/**
 * @brief Draw a string
 * @param x X coordinate
 * @param y Y coordinate
 * @param str String
 */
void DRAW_String(uint8_t x, uint8_t y, const char *str);

/**
 * @brief Draw inverted string (white on black)
 * @param x X coordinate
 * @param y Y coordinate
 * @param str String
 */
void DRAW_StringInvert(uint8_t x, uint8_t y, const char *str);

/**
 * @brief Draw centered string
 * @param y Y coordinate
 * @param str String
 */
void DRAW_StringCenter(uint8_t y, const char *str);

/*============================================================================
 * Bitmap Functions
 *===========================================================================*/

/**
 * @brief Draw a bitmap
 * @param x X coordinate
 * @param y Y coordinate
 * @param width Bitmap width
 * @param height Bitmap height
 * @param bitmap Pointer to bitmap data
 */
void DRAW_Bitmap(uint8_t x, uint8_t y, uint8_t width, uint8_t height,
                 const uint8_t *bitmap);

/**
 * @brief Draw a bitmap with transparency (0 = transparent)
 * @param x X coordinate
 * @param y Y coordinate
 * @param width Bitmap width
 * @param height Bitmap height
 * @param bitmap Pointer to bitmap data
 */
void DRAW_BitmapTransparent(uint8_t x, uint8_t y, uint8_t width, uint8_t height,
                            const uint8_t *bitmap);

/*============================================================================
 * Progress Bar / Meter Functions
 *===========================================================================*/

/**
 * @brief Draw a progress bar
 * @param x X coordinate
 * @param y Y coordinate
 * @param width Total width
 * @param height Height
 * @param percent Percentage (0-100)
 */
void DRAW_ProgressBar(uint8_t x, uint8_t y, uint8_t width, uint8_t height,
                      uint8_t percent);

/**
 * @brief Draw a vertical bar
 * @param x X coordinate
 * @param y_base Base Y coordinate
 * @param width Bar width
 * @param max_height Maximum height
 * @param value Current value
 * @param max_value Maximum value
 */
void DRAW_VerticalBar(uint8_t x, uint8_t y_base, uint8_t width,
                      uint8_t max_height, uint8_t value, uint8_t max_value);

/*============================================================================
 * Screen Effects
 *===========================================================================*/

/**
 * @brief Draw a frame/border around screen
 */
void DRAW_ScreenFrame(void);

/**
 * @brief Draw header bar with title
 * @param title Title text
 */
void DRAW_HeaderBar(const char *title);

/**
 * @brief Draw footer bar with text
 * @param text Footer text
 */
void DRAW_FooterBar(const char *text);

/**
 * @brief Draw a dialog box
 * @param title Title text (or NULL)
 * @param message Message text
 */
void DRAW_DialogBox(const char *title, const char *message);

#endif /* LCD_GRAPHIC_H_ */