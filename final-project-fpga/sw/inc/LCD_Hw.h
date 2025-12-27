/******************************************************************************
 * LCD_Hw.h
 * 
 * LCD Hardware Abstraction Layer for KS0108B controller
 * Low-level hardware interface definitions
 *
 ******************************************************************************/

#ifndef LCD_HW_H_
#define LCD_HW_H_

#include "terasic_os_includes.h"

/*============================================================================
 * LCD Hardware Configuration
 *===========================================================================*/

/* LCD Physical Dimensions */
#define LCD_WIDTH           128     /* Pixels horizontally */
#define LCD_HEIGHT          64      /* Pixels vertically */
#define LCD_PAGES           8       /* Vertical pages (64/8) */
#define LCD_PAGE_HEIGHT     8       /* Pixels per page */

/* LCD Controllers (KS0108B uses dual controllers) */
#define LCD_CTRL_LEFT       0       /* Left half (columns 0-63) */
#define LCD_CTRL_RIGHT      1       /* Right half (columns 64-127) */
#define LCD_COLS_PER_CTRL   64      /* Columns per controller */

/* KS0108B Commands */
#define LCD_CMD_DISPLAY_ON  0x3F    /* Display ON */
#define LCD_CMD_DISPLAY_OFF 0x3E    /* Display OFF */
#define LCD_CMD_SET_Y       0x40    /* Set Y address (column within controller) */
#define LCD_CMD_SET_X       0xB8    /* Set X address (page 0-7) */
#define LCD_CMD_SET_Z       0xC0    /* Set Z address (start line) */

/* Command/Data Selection */
#define LCD_RS_COMMAND      0       /* RS=0 for command */
#define LCD_RS_DATA         1       /* RS=1 for data */

/* Read/Write Selection */
#define LCD_RW_WRITE        0       /* RW=0 for write */
#define LCD_RW_READ         1       /* RW=1 for read */

/*============================================================================
 * LCD Timing Parameters (from KS0108B datasheet)
 *===========================================================================*/

/* All times in nanoseconds */
#define LCD_T_CYCLE         1000    /* E cycle time (min 1000ns) */
#define LCD_T_PW_EH         450     /* E high pulse width (min 450ns) */
#define LCD_T_PW_EL         450     /* E low pulse width (min 450ns) */
#define LCD_T_AS            140     /* Address setup time (min 140ns) */
#define LCD_T_AH            10      /* Address hold time (min 10ns) */
#define LCD_T_DSW           200     /* Data setup time for write (min 200ns) */
#define LCD_T_DHW           10      /* Data hold time for write (min 10ns) */

/* Timing in microseconds for software delays */
#define LCD_DELAY_US        2       /* General delay */
#define LCD_SETUP_DELAY_US  1       /* Setup delay */
#define LCD_PULSE_DELAY_US  1       /* Pulse width delay */

/*============================================================================
 * Hardware Interface Functions
 *===========================================================================*/

/**
 * @brief Initialize LCD hardware interface
 * @return 0 on success, -1 on failure
 */
int LCD_HW_Init(void);

/**
 * @brief Deinitialize LCD hardware
 */
void LCD_HW_DeInit(void);

/**
 * @brief Write data byte to LCD
 * @param controller Which controller (LCD_CTRL_LEFT or LCD_CTRL_RIGHT)
 * @param rs Register select (LCD_RS_COMMAND or LCD_RS_DATA)
 * @param data Data byte to write
 */
void LCD_HW_Write(uint8_t controller, uint8_t rs, uint8_t data);

/**
 * @brief Read data byte from LCD
 * @param controller Which controller (LCD_CTRL_LEFT or LCD_CTRL_RIGHT)
 * @param rs Register select (LCD_RS_COMMAND or LCD_RS_DATA)
 * @return Data byte read
 */
uint8_t LCD_HW_Read(uint8_t controller, uint8_t rs);

/**
 * @brief Write command to LCD
 * @param controller Which controller
 * @param cmd Command byte
 */
void LCD_HW_WriteCmd(uint8_t controller, uint8_t cmd);

/**
 * @brief Write data to LCD
 * @param controller Which controller
 * @param data Data byte
 */
void LCD_HW_WriteData(uint8_t controller, uint8_t data);

/**
 * @brief Read status from LCD
 * @param controller Which controller
 * @return Status byte
 */
uint8_t LCD_HW_ReadStatus(uint8_t controller);

/**
 * @brief Wait for LCD to be ready
 * @param controller Which controller
 */
void LCD_HW_WaitReady(uint8_t controller);

/**
 * @brief Set chip select for controller
 * @param controller Which controller (or -1 for none)
 */
void LCD_HW_SetChipSelect(int controller);

/**
 * @brief Control backlight
 * @param on 1 for on, 0 for off
 */
void LCD_HW_Backlight(int on);

/**
 * @brief Hardware reset of LCD
 */
void LCD_HW_Reset(void);

/**
 * @brief Delay for LCD timing requirements
 * @param us Microseconds to delay
 */
void LCD_HW_DelayUs(uint32_t us);

/*============================================================================
 * FPGA Interface (when using FPGA LCD controller)
 *===========================================================================*/

/**
 * @brief Check if FPGA LCD controller is ready
 * @return 1 if ready, 0 if busy
 */
int LCD_HW_FPGA_Ready(void);

/**
 * @brief Write to FPGA LCD controller
 * @param reg_offset Register offset
 * @param value Value to write
 */
void LCD_HW_FPGA_Write(uint32_t reg_offset, uint32_t value);

/**
 * @brief Read from FPGA LCD controller
 * @param reg_offset Register offset
 * @return Value read
 */
uint32_t LCD_HW_FPGA_Read(uint32_t reg_offset);

#endif /* LCD_HW_H_ */