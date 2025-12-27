/******************************************************************************
 * addresses.h
 * 
 * Memory Map Definitions for FPGA Peripherals
 * Auto-generated from Platform Designer (Qsys)
 * 
 * WARNING: This file should only be modified after Qsys regeneration
 *
 ******************************************************************************/

#ifndef ADDRESSES_H_
#define ADDRESSES_H_

#include "terasic_os_includes.h"

/*============================================================================
 * FPGA Bridge Configuration
 *===========================================================================*/

/* Lightweight HPS-to-FPGA Bridge */
#define FPGA_LW_BRIDGE_BASE         0xFF200000
#define FPGA_LW_BRIDGE_SPAN         0x00200000  /* 2 MB */

/* Full HPS-to-FPGA Bridge (if used) */
#define FPGA_H2F_BRIDGE_BASE        0xC0000000
#define FPGA_H2F_BRIDGE_SPAN        0x04000000  /* 64 MB */

/*============================================================================
 * LCD Controller Registers
 * Base: 0xFF200000, Span: 0x20 (32 bytes)
 *===========================================================================*/

#define LCD_CONTROLLER_BASE         0x00000000  /* Offset from LW Bridge */

/* Register Offsets */
#define LCD_DATA_REG_OFFSET         0x00    /* Write: Data to display */
#define LCD_CMD_REG_OFFSET          0x04    /* Write: Command register */
#define LCD_STATUS_REG_OFFSET       0x08    /* Read: Status register */
#define LCD_CTRL_REG_OFFSET         0x0C    /* Read/Write: Control register */
#define LCD_X_POS_REG_OFFSET        0x10    /* Write: X position (0-127) */
#define LCD_Y_POS_REG_OFFSET        0x14    /* Write: Y position/page (0-7) */
#define LCD_PIXEL_REG_OFFSET        0x18    /* Write: Direct pixel data */

/* Status Register Bits */
#define LCD_STATUS_BUSY             (1 << 0)    /* LCD controller busy */
#define LCD_STATUS_INIT_DONE        (1 << 1)    /* Initialization complete */
#define LCD_STATUS_ERROR            (1 << 7)    /* Error flag */

/* Control Register Bits */
#define LCD_CTRL_ENABLE             (1 << 0)    /* Enable display */
#define LCD_CTRL_RESET              (1 << 1)    /* Soft reset */
#define LCD_CTRL_BACKLIGHT          (1 << 2)    /* Backlight control */
#define LCD_CTRL_INVERT             (1 << 3)    /* Invert display */

/*============================================================================
 * Button/Key Interface
 * Base: 0xFF200020, Span: 0x10 (16 bytes)
 *===========================================================================*/

#define BUTTON_BASE                 0x00000020  /* Offset from LW Bridge */

/* Register Offsets */
#define BUTTON_DATA_REG_OFFSET      0x00    /* Read: Current button state */
#define BUTTON_EDGE_REG_OFFSET      0x04    /* Read/Clear: Edge capture */
#define BUTTON_IRQ_MASK_OFFSET      0x08    /* Read/Write: IRQ mask */

/* Button Bit Definitions */
#define BUTTON_KEY0                 (1 << 0)    /* KEY0 - Next customer */
#define BUTTON_KEY1                 (1 << 1)    /* KEY1 - Add customer */
#define BUTTON_KEY2                 (1 << 2)    /* KEY2 - Toggle screen */
#define BUTTON_KEY3                 (1 << 3)    /* KEY3 - Reset system */

/*============================================================================
 * Switch Interface
 * Base: 0xFF200030, Span: 0x10 (16 bytes)
 *===========================================================================*/

#define SWITCH_BASE                 0x00000030  /* Offset from LW Bridge */

/* Register Offsets */
#define SWITCH_DATA_REG_OFFSET      0x00    /* Read: Current switch state */

/*============================================================================
 * LED Interface
 * Base: 0xFF200040, Span: 0x10 (16 bytes)
 *===========================================================================*/

#define LED_BASE                    0x00000040  /* Offset from LW Bridge */

/* Register Offsets */
#define LED_DATA_REG_OFFSET         0x00    /* Write: LED pattern */

/*============================================================================
 * 7-Segment Display Interface
 * Base: 0xFF200050, Span: 0x20 (32 bytes)
 *===========================================================================*/

#define HEX_BASE                    0x00000050  /* Offset from LW Bridge */

/* Register Offsets */
#define HEX0_3_REG_OFFSET           0x00    /* Write: HEX0-3 pattern */
#define HEX4_5_REG_OFFSET           0x04    /* Write: HEX4-5 pattern */

/*============================================================================
 * Programmable Timer
 * Base: 0xFF200070, Span: 0x10 (16 bytes)
 *===========================================================================*/

#define TIMER_BASE                  0x00000070  /* Offset from LW Bridge */

/* Register Offsets */
#define TIMER_LOAD_REG_OFFSET       0x00    /* Write: Load value */
#define TIMER_COUNT_REG_OFFSET      0x04    /* Read: Current count */
#define TIMER_CTRL_REG_OFFSET       0x08    /* Read/Write: Control */
#define TIMER_STATUS_REG_OFFSET     0x0C    /* Read/Clear: Status */

/* Control Register Bits */
#define TIMER_CTRL_ENABLE           (1 << 0)    /* Enable timer */
#define TIMER_CTRL_IRQ_EN           (1 << 1)    /* Enable interrupt */
#define TIMER_CTRL_CONT             (1 << 2)    /* Continuous mode */

/* Status Register Bits */
#define TIMER_STATUS_TIMEOUT        (1 << 0)    /* Timeout occurred */
#define TIMER_STATUS_RUNNING        (1 << 1)    /* Timer running */

/*============================================================================
 * UART Transmitter
 * Base: 0xFF200080, Span: 0x10 (16 bytes)
 *===========================================================================*/

#define UART_BASE                   0x00000080  /* Offset from LW Bridge */

/* Register Offsets */
#define UART_DATA_REG_OFFSET        0x00    /* Write: TX data */
#define UART_STATUS_REG_OFFSET      0x04    /* Read: Status */
#define UART_CTRL_REG_OFFSET        0x08    /* Read/Write: Control */

/* Status Register Bits */
#define UART_STATUS_TX_BUSY         (1 << 0)    /* Transmitter busy */
#define UART_STATUS_TX_EMPTY        (1 << 1)    /* TX buffer empty */

/*============================================================================
 * Message Memory (ROM)
 * Base: 0xFF200090, Span: 0x100 (256 bytes)
 *===========================================================================*/

#define MESSAGE_MEM_BASE            0x00000090  /* Offset from LW Bridge */

/* 16 messages x 16 bytes each = 256 bytes */
#define MESSAGE_COUNT               16
#define MESSAGE_LENGTH              16

/*============================================================================
 * Queue System Registers
 * Base: 0xFF200190, Span: 0x20 (32 bytes)
 *===========================================================================*/

#define QUEUE_BASE                  0x00000190  /* Offset from LW Bridge */

/* Register Offsets */
#define QUEUE_CURRENT_REG_OFFSET    0x00    /* Read/Write: Current serving */
#define QUEUE_NEXT_REG_OFFSET       0x04    /* Read: Next ticket number */
#define QUEUE_WAITING_REG_OFFSET    0x08    /* Read: Waiting count */
#define QUEUE_CTRL_REG_OFFSET       0x0C    /* Write: Control commands */
#define QUEUE_STATUS_REG_OFFSET     0x10    /* Read: Queue status */

/* Control Register Commands */
#define QUEUE_CTRL_NEXT             (1 << 0)    /* Call next customer */
#define QUEUE_CTRL_ADD              (1 << 1)    /* Add new customer */
#define QUEUE_CTRL_RESET            (1 << 2)    /* Reset queue */

/* Status Register Bits */
#define QUEUE_STATUS_EMPTY          (1 << 0)    /* Queue is empty */
#define QUEUE_STATUS_FULL           (1 << 1)    /* Queue is full */

/*============================================================================
 * System ID / Timestamp
 * Base: 0xFF2001B0, Span: 0x08 (8 bytes)
 *===========================================================================*/

#define SYSID_BASE                  0x000001B0  /* Offset from LW Bridge */

#define SYSID_ID_REG_OFFSET         0x00    /* Read: System ID */
#define SYSID_TIME_REG_OFFSET       0x04    /* Read: Build timestamp */

/*============================================================================
 * Address Calculation Macros
 *===========================================================================*/

/* Calculate absolute address from bridge base and peripheral offset */
#define ABS_ADDR(bridge_base, peripheral_offset) \
    ((bridge_base) + (peripheral_offset))

/* Shorthand macros for common accesses */
#define LCD_ADDR(offset)    ABS_ADDR(FPGA_LW_BRIDGE_BASE, LCD_CONTROLLER_BASE + (offset))
#define BTN_ADDR(offset)    ABS_ADDR(FPGA_LW_BRIDGE_BASE, BUTTON_BASE + (offset))
#define LED_ADDR(offset)    ABS_ADDR(FPGA_LW_BRIDGE_BASE, LED_BASE + (offset))
#define TIMER_ADDR(offset)  ABS_ADDR(FPGA_LW_BRIDGE_BASE, TIMER_BASE + (offset))
#define QUEUE_ADDR(offset)  ABS_ADDR(FPGA_LW_BRIDGE_BASE, QUEUE_BASE + (offset))

#endif /* ADDRESSES_H_ */