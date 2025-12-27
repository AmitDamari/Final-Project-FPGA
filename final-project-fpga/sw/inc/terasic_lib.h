/******************************************************************************
 * terasic_lib.h
 * 
 * Terasic Library - Common utility functions for DE10-Standard
 *
 ******************************************************************************/

#ifndef TERASIC_LIB_H_
#define TERASIC_LIB_H_

#include "terasic_os_includes.h"

/*============================================================================
 * Memory Mapping Functions
 *===========================================================================*/

/**
 * @brief Open /dev/mem and map FPGA bridge to virtual memory
 * @return Pointer to mapped memory, NULL on failure
 */
void *TERASIC_MEM_Open(void);

/**
 * @brief Unmap FPGA bridge and close /dev/mem
 * @param virtual_base Pointer returned by TERASIC_MEM_Open()
 */
void TERASIC_MEM_Close(void *virtual_base);

/**
 * @brief Get virtual address for lightweight bridge
 * @return Pointer to LW bridge base
 */
void *TERASIC_GetLWBridgeBase(void);

/**
 * @brief Get virtual address for H2F bridge
 * @return Pointer to H2F bridge base
 */
void *TERASIC_GetH2FBridgeBase(void);

/*============================================================================
 * Time Functions
 *===========================================================================*/

/**
 * @brief Get current tick count in milliseconds
 * @return Milliseconds since system start
 */
uint32_t get_tick_count(void);

/**
 * @brief Sleep for specified milliseconds
 * @param ms Milliseconds to sleep
 */
void msleep(uint32_t ms);

/**
 * @brief Sleep for specified microseconds (with tolerance)
 * @param min_us Minimum microseconds
 * @param max_us Maximum microseconds
 */
void usleep_range(uint32_t min_us, uint32_t max_us);

/**
 * @brief Get high-resolution timestamp
 * @return Timestamp in microseconds
 */
uint64_t get_timestamp_us(void);

/*============================================================================
 * GPIO Functions
 *===========================================================================*/

/**
 * @brief Initialize GPIO for user LEDs
 * @return 0 on success, -1 on failure
 */
int TERASIC_GPIO_Init(void);

/**
 * @brief Set LED pattern
 * @param led_pattern Bit pattern for LEDs (bit 0 = LED0, etc.)
 */
void TERASIC_LED_Set(uint32_t led_pattern);

/**
 * @brief Get current LED pattern
 * @return Current LED pattern
 */
uint32_t TERASIC_LED_Get(void);

/**
 * @brief Read button states
 * @return Button states (bit 0 = KEY0, etc.) - active when pressed
 */
uint32_t TERASIC_BUTTON_Read(void);

/**
 * @brief Read switch states
 * @return Switch states (bit 0 = SW0, etc.)
 */
uint32_t TERASIC_SWITCH_Read(void);

/*============================================================================
 * 7-Segment Display Functions
 *===========================================================================*/

/**
 * @brief Display hex value on 7-segment displays
 * @param display_num Display number (0-5)
 * @param value Value to display (0-15)
 */
void TERASIC_HEX_Set(int display_num, uint8_t value);

/**
 * @brief Display number on multiple 7-segment displays
 * @param number Number to display
 * @param num_digits Number of digits to use
 */
void TERASIC_HEX_Display(uint32_t number, int num_digits);

/**
 * @brief Clear all 7-segment displays
 */
void TERASIC_HEX_Clear(void);

/*============================================================================
 * Utility Functions
 *===========================================================================*/

/**
 * @brief Print buffer as hex dump
 * @param data Pointer to data
 * @param len Length of data
 */
void TERASIC_HexDump(const void *data, size_t len);

/**
 * @brief Debug print with timestamp
 * @param format Printf format string
 */
void TERASIC_DebugPrint(const char *format, ...);

#endif /* TERASIC_LIB_H_ */