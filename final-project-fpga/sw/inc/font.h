/******************************************************************************
 * font.h
 * 
 * Font definitions for LCD graphics
 * 5x7 pixel font for 128x64 LCD display
 *
 ******************************************************************************/

#ifndef FONT_H_
#define FONT_H_

#include "terasic_os_includes.h"

/*============================================================================
 * Font Configuration
 *===========================================================================*/

#define FONT_WIDTH          5       /* Character width in pixels */
#define FONT_HEIGHT         7       /* Character height in pixels */
#define FONT_SPACING        1       /* Spacing between characters */
#define FONT_FIRST_CHAR     0x20    /* First printable ASCII character (space) */
#define FONT_LAST_CHAR      0x7E    /* Last printable ASCII character (~) */
#define FONT_CHAR_COUNT     (FONT_LAST_CHAR - FONT_FIRST_CHAR + 1)

/* Character cell dimensions (including spacing) */
#define CHAR_WIDTH          (FONT_WIDTH + FONT_SPACING)
#define CHAR_HEIGHT         (FONT_HEIGHT + 1)

/* Characters per LCD row/column */
#define LCD_CHAR_COLS       (128 / CHAR_WIDTH)      /* 21 characters */
#define LCD_CHAR_ROWS       (64 / CHAR_HEIGHT)      /* 8 rows */

/*============================================================================
 * Large Font Configuration (for queue numbers)
 *===========================================================================*/

#define LARGE_FONT_WIDTH    8       /* Large character width */
#define LARGE_FONT_HEIGHT   14      /* Large character height */
#define LARGE_DIGIT_COUNT   10      /* Digits 0-9 */
#define LARGE_LETTER_COUNT  26      /* Letters A-Z */

/*============================================================================
 * Font Data Declarations
 *===========================================================================*/

/**
 * Standard 5x7 font bitmap array
 * Each character is 5 bytes (columns), each byte represents 7 vertical pixels
 */
extern const uint8_t font_5x7[FONT_CHAR_COUNT][FONT_WIDTH];

/**
 * Large digit font for queue display (8x14)
 * Each digit is 14 bytes (rows)
 */
extern const uint8_t font_large_digits[LARGE_DIGIT_COUNT][LARGE_FONT_HEIGHT];

/**
 * Large letter font for queue prefix (8x14)
 * Letters A-Z
 */
extern const uint8_t font_large_letters[LARGE_LETTER_COUNT][LARGE_FONT_HEIGHT];

/*============================================================================
 * Font Access Functions
 *===========================================================================*/

/**
 * @brief Get font data for a character
 * @param c ASCII character
 * @return Pointer to 5-byte font data, NULL if invalid
 */
const uint8_t *Font_GetChar(char c);

/**
 * @brief Get large digit font data
 * @param digit Digit 0-9
 * @return Pointer to 14-byte font data, NULL if invalid
 */
const uint8_t *Font_GetLargeDigit(uint8_t digit);

/**
 * @brief Get large letter font data
 * @param letter Letter 'A'-'Z'
 * @return Pointer to 14-byte font data, NULL if invalid
 */
const uint8_t *Font_GetLargeLetter(char letter);

/**
 * @brief Get character width (accounts for variable-width if implemented)
 * @param c ASCII character
 * @return Width in pixels
 */
uint8_t Font_GetCharWidth(char c);

/**
 * @brief Calculate string width in pixels
 * @param str String to measure
 * @return Total width in pixels
 */
uint16_t Font_GetStringWidth(const char *str);

#endif /* FONT_H_ */