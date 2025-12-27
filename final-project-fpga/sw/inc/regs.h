/******************************************************************************
 * regs.h
 * 
 * Register Bit-field Definitions and Access Macros
 *
 ******************************************************************************/

#ifndef REGS_H_
#define REGS_H_

#include "terasic_os_includes.h"
#include "addresses.h"

/*============================================================================
 * Bit Manipulation Macros
 *===========================================================================*/

#define BIT(n)                      (1UL << (n))
#define BIT_MASK(n)                 (BIT(n) - 1)
#define BITS(hi, lo)                ((BIT((hi) - (lo) + 1) - 1) << (lo))

#define SET_BIT(reg, bit)           ((reg) |= BIT(bit))
#define CLR_BIT(reg, bit)           ((reg) &= ~BIT(bit))
#define TST_BIT(reg, bit)           (((reg) & BIT(bit)) != 0)
#define TOG_BIT(reg, bit)           ((reg) ^= BIT(bit))

#define GET_FIELD(reg, mask, shift) (((reg) & (mask)) >> (shift))
#define SET_FIELD(reg, mask, shift, val) \
    ((reg) = (((reg) & ~(mask)) | (((val) << (shift)) & (mask))))

/*============================================================================
 * LCD Controller Register Structures
 *===========================================================================*/

/* LCD Status Register Bits */
typedef union {
    struct {
        uint32_t busy       : 1;    /* Bit 0: Controller busy */
        uint32_t init_done  : 1;    /* Bit 1: Initialization complete */
        uint32_t reserved   : 5;    /* Bits 2-6: Reserved */
        uint32_t error      : 1;    /* Bit 7: Error flag */
        uint32_t unused     : 24;   /* Bits 8-31: Unused */
    } bits;
    uint32_t value;
} lcd_status_reg_t;

/* LCD Control Register Bits */
typedef union {
    struct {
        uint32_t enable     : 1;    /* Bit 0: Enable display */
        uint32_t reset      : 1;    /* Bit 1: Soft reset */
        uint32_t backlight  : 1;    /* Bit 2: Backlight on/off */
        uint32_t invert     : 1;    /* Bit 3: Invert display */
        uint32_t reserved   : 28;   /* Bits 4-31: Reserved */
    } bits;
    uint32_t value;
} lcd_ctrl_reg_t;

/*============================================================================
 * Timer Register Structures
 *===========================================================================*/

/* Timer Control Register Bits */
typedef union {
    struct {
        uint32_t enable     : 1;    /* Bit 0: Enable timer */
        uint32_t irq_en     : 1;    /* Bit 1: Enable interrupt */
        uint32_t continuous : 1;    /* Bit 2: Continuous mode */
        uint32_t reserved   : 29;   /* Bits 3-31: Reserved */
    } bits;
    uint32_t value;
} timer_ctrl_reg_t;

/* Timer Status Register Bits */
typedef union {
    struct {
        uint32_t timeout    : 1;    /* Bit 0: Timeout occurred */
        uint32_t running    : 1;    /* Bit 1: Timer running */
        uint32_t reserved   : 30;   /* Bits 2-31: Reserved */
    } bits;
    uint32_t value;
} timer_status_reg_t;

/*============================================================================
 * UART Register Structures
 *===========================================================================*/

/* UART Status Register Bits */
typedef union {
    struct {
        uint32_t tx_busy    : 1;    /* Bit 0: Transmitter busy */
        uint32_t tx_empty   : 1;    /* Bit 1: TX buffer empty */
        uint32_t reserved   : 30;   /* Bits 2-31: Reserved */
    } bits;
    uint32_t value;
} uart_status_reg_t;

/*============================================================================
 * Queue Register Structures
 *===========================================================================*/

/* Queue Control Register Commands */
typedef union {
    struct {
        uint32_t call_next  : 1;    /* Bit 0: Call next customer */
        uint32_t add_new    : 1;    /* Bit 1: Add new customer */
        uint32_t reset      : 1;    /* Bit 2: Reset queue */
        uint32_t reserved   : 29;   /* Bits 3-31: Reserved */
    } bits;
    uint32_t value;
} queue_ctrl_reg_t;

/* Queue Status Register Bits */
typedef union {
    struct {
        uint32_t empty      : 1;    /* Bit 0: Queue empty */
        uint32_t full       : 1;    /* Bit 1: Queue full */
        uint32_t reserved   : 30;   /* Bits 2-31: Reserved */
    } bits;
    uint32_t value;
} queue_status_reg_t;

/*============================================================================
 * Button/Key Register Structures
 *===========================================================================*/

/* Button Data Register Bits */
typedef union {
    struct {
        uint32_t key0       : 1;    /* Bit 0: KEY0 state */
        uint32_t key1       : 1;    /* Bit 1: KEY1 state */
        uint32_t key2       : 1;    /* Bit 2: KEY2 state */
        uint32_t key3       : 1;    /* Bit 3: KEY3 state */
        uint32_t reserved   : 28;   /* Bits 4-31: Reserved */
    } bits;
    uint32_t value;
} button_data_reg_t;

/*============================================================================
 * 7-Segment Display Encoding
 *===========================================================================*/

/* Segment encoding: gfedcba (active low for DE10-Standard) */
static const uint8_t hex_segments[16] = {
    0x40, /* 0: 0b1000000 */
    0x79, /* 1: 0b1111001 */
    0x24, /* 2: 0b0100100 */
    0x30, /* 3: 0b0110000 */
    0x19, /* 4: 0b0011001 */
    0x12, /* 5: 0b0010010 */
    0x02, /* 6: 0b0000010 */
    0x78, /* 7: 0b1111000 */
    0x00, /* 8: 0b0000000 */
    0x10, /* 9: 0b0010000 */
    0x08, /* A: 0b0001000 */
    0x03, /* B: 0b0000011 */
    0x46, /* C: 0b1000110 */
    0x21, /* D: 0b0100001 */
    0x06, /* E: 0b0000110 */
    0x0E  /* F: 0b0001110 */
};

#define HEX_BLANK   0x7F    /* All segments off */
#define HEX_DASH    0x3F    /* Only middle segment */

#endif /* REGS_H_ */