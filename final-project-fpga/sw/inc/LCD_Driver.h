/******************************************************************************
 * lcd_driver.h
 * 
 * High-level LCD Driver API for Smart Queue System
 * Integrates with FPGA LCD controller
 *
 ******************************************************************************/

#ifndef LCD_DRIVER_API_H_
#define LCD_DRIVER_API_H_

#include "terasic_os_includes.h"

/*============================================================================
 * LCD Driver API - Initialization
 *===========================================================================*/

/**
 * @brief Initialize LCD subsystem
 * @return 0 on success, -1 on failure
 */
int lcd_init(void);

/**
 * @brief Deinitialize LCD subsystem
 */
void lcd_deinit(void);

/**
 * @brief Check if LCD is ready
 * @return 1 if ready, 0 if busy
 */
int lcd_ready(void);

/**
 * @brief Reset LCD controller
 */
void lcd_reset(void);

/*============================================================================
 * LCD Driver API - Display Control
 *===========================================================================*/

/**
 * @brief Clear display
 */
void lcd_clear(void);

/**
 * @brief Update display (refresh from buffer)
 */
void lcd_update(void);

/**
 * @brief Turn display on/off
 * @param on 1=on, 0=off
 */
void lcd_display_on(int on);

/**
 * @brief Control backlight
 * @param on 1=on, 0=off
 */
void lcd_backlight(int on);

/*============================================================================
 * LCD Driver API - Text Output
 *===========================================================================*/

/**
 * @brief Print string at position
 * @param x X coordinate
 * @param y Y coordinate
 * @param str String to print
 */
void lcd_print(uint8_t x, uint8_t y, const char *str);

/**
 * @brief Print formatted string
 * @param x X coordinate
 * @param y Y coordinate
 * @param fmt Format string
 */
void lcd_printf(uint8_t x, uint8_t y, const char *fmt, ...);

/**
 * @brief Print centered string
 * @param y Y coordinate
 * @param str String to print
 */
void lcd_print_center(uint8_t y, const char *str);

/**
 * @brief Print large text (for queue numbers)
 * @param x X coordinate
 * @param y Y coordinate
 * @param str String to print
 */
void lcd_print_large(uint8_t x, uint8_t y, const char *str);

/*============================================================================
 * LCD Driver API - Graphics
 *===========================================================================*/

/**
 * @brief Draw pixel
 * @param x X coordinate
 * @param y Y coordinate
 * @param color 0=off, 1=on
 */
void lcd_pixel(uint8_t x, uint8_t y, uint8_t color);

/**
 * @brief Draw line
 * @param x0 Start X
 * @param y0 Start Y
 * @param x1 End X
 * @param y1 End Y
 */
void lcd_line(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1);

/**
 * @brief Draw rectangle
 * @param x X coordinate
 * @param y Y coordinate
 * @param w Width
 * @param h Height
 * @param fill 1=filled, 0=outline
 */
void lcd_rect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t fill);

/**
 * @brief Draw progress bar
 * @param x X coordinate
 * @param y Y coordinate
 * @param w Width
 * @param h Height
 * @param percent Percentage (0-100)
 */
void lcd_progress_bar(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t percent);

/**
 * @brief Draw bitmap image
 * @param x X coordinate
 * @param y Y coordinate
 * @param w Width
 * @param h Height
 * @param bitmap Pointer to bitmap data
 */
void lcd_bitmap(uint8_t x, uint8_t y, uint8_t w, uint8_t h, const uint8_t *bitmap);

/*============================================================================
 * LCD Driver API - FPGA Interface
 *===========================================================================*/

/**
 * @brief Write to FPGA LCD controller register
 * @param offset Register offset
 * @param value Value to write
 */
void lcd_fpga_write(uint32_t offset, uint32_t value);

/**
 * @brief Read from FPGA LCD controller register
 * @param offset Register offset
 * @return Value read
 */
uint32_t lcd_fpga_read(uint32_t offset);

/**
 * @brief Get FPGA controller status
 * @return Status register value
 */
uint32_t lcd_fpga_status(void);

#endif /* LCD_DRIVER_API_H_ */