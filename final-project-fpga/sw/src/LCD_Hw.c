/******************************************************************************
 * LCD_Hw.c
 * 
 * LCD Hardware Abstraction Layer for KS0108B controller
 * Implements low-level communication with LCD via FPGA or GPIO
 *
 ******************************************************************************/

#include "LCD_Hw.h"
#include "terasic_lib.h"
#include "addresses.h"

/*============================================================================
 * Private Variables
 *===========================================================================*/

static void *g_fpga_base = NULL;
static bool g_hw_initialized = false;

/*============================================================================
 * Hardware Interface Implementation
 *===========================================================================*/

int LCD_HW_Init(void)
{
    if (g_hw_initialized) {
        return 0;
    }
    
    /* Get FPGA bridge base address */
    g_fpga_base = TERASIC_GetLWBridgeBase();
    if (g_fpga_base == NULL) {
        ERROR_PRINT("Failed to get FPGA base address");
        return -1;
    }
    
    /* Perform hardware reset */
    LCD_HW_Reset();
    
    /* Wait for LCD to stabilize */
    LCD_HW_DelayUs(50000);  /* 50ms */
    
    g_hw_initialized = true;
    INFO_PRINT("LCD hardware initialized");
    
    return 0;
}

void LCD_HW_DeInit(void)
{
    if (!g_hw_initialized) {
        return;
    }
    
    /* Turn off display */
    LCD_HW_WriteCmd(LCD_CTRL_LEFT, LCD_CMD_DISPLAY_OFF);
    LCD_HW_WriteCmd(LCD_CTRL_RIGHT, LCD_CMD_DISPLAY_OFF);
    
    g_hw_initialized = false;
    g_fpga_base = NULL;
}

void LCD_HW_Write(uint8_t controller, uint8_t rs, uint8_t data)
{
    if (g_fpga_base == NULL) return;
    
    volatile uint32_t *lcd_base = (volatile uint32_t *)
        ((uint8_t *)g_fpga_base + LCD_CONTROLLER_BASE);
    
    /* Wait for controller ready */
    while (lcd_base[LCD_STATUS_REG_OFFSET/4] & LCD_STATUS_BUSY) {
        LCD_HW_DelayUs(1);
    }
    
    /* Select controller via control register */
    uint32_t ctrl = lcd_base[LCD_CTRL_REG_OFFSET/4];
    ctrl &= ~0x30;  /* Clear CS bits */
    ctrl |= (controller == LCD_CTRL_LEFT) ? 0x10 : 0x20;
    lcd_base[LCD_CTRL_REG_OFFSET/4] = ctrl;
    
    /* Write data or command */
    if (rs == LCD_RS_DATA) {
        lcd_base[LCD_DATA_REG_OFFSET/4] = data;
    } else {
        lcd_base[LCD_CMD_REG_OFFSET/4] = data;
    }
}

uint8_t LCD_HW_Read(uint8_t controller, uint8_t rs)
{
    if (g_fpga_base == NULL) return 0;
    
    volatile uint32_t *lcd_base = (volatile uint32_t *)
        ((uint8_t *)g_fpga_base + LCD_CONTROLLER_BASE);
    
    /* Wait for controller ready */
    while (lcd_base[LCD_STATUS_REG_OFFSET/4] & LCD_STATUS_BUSY) {
        LCD_HW_DelayUs(1);
    }
    
    /* Select controller */
    uint32_t ctrl = lcd_base[LCD_CTRL_REG_OFFSET/4];
    ctrl &= ~0x30;
    ctrl |= (controller == LCD_CTRL_LEFT) ? 0x10 : 0x20;
    lcd_base[LCD_CTRL_REG_OFFSET/4] = ctrl;
    
    /* Read data or status */
    if (rs == LCD_RS_DATA) {
        return (uint8_t)(lcd_base[LCD_DATA_REG_OFFSET/4] & 0xFF);
    } else {
        return (uint8_t)(lcd_base[LCD_STATUS_REG_OFFSET/4] & 0xFF);
    }
}

void LCD_HW_WriteCmd(uint8_t controller, uint8_t cmd)
{
    LCD_HW_Write(controller, LCD_RS_COMMAND, cmd);
}

void LCD_HW_WriteData(uint8_t controller, uint8_t data)
{
    LCD_HW_Write(controller, LCD_RS_DATA, data);
}

uint8_t LCD_HW_ReadStatus(uint8_t controller)
{
    return LCD_HW_Read(controller, LCD_RS_COMMAND);
}

void LCD_HW_WaitReady(uint8_t controller)
{
    int timeout = 10000;  /* 10ms timeout */
    
    while (timeout > 0) {
        uint8_t status = LCD_HW_ReadStatus(controller);
        if ((status & 0x80) == 0) {  /* Busy flag clear */
            return;
        }
        LCD_HW_DelayUs(1);
        timeout--;
    }
    
    WARNING_PRINT("LCD busy timeout on controller %d", controller);
}

void LCD_HW_SetChipSelect(int controller)
{
    if (g_fpga_base == NULL) return;
    
    volatile uint32_t *lcd_base = (volatile uint32_t *)
        ((uint8_t *)g_fpga_base + LCD_CONTROLLER_BASE);
    
    uint32_t ctrl = lcd_base[LCD_CTRL_REG_OFFSET/4];
    ctrl &= ~0x30;  /* Clear both CS bits */
    
    if (controller == LCD_CTRL_LEFT) {
        ctrl |= 0x10;
    } else if (controller == LCD_CTRL_RIGHT) {
        ctrl |= 0x20;
    }
    /* If controller == -1, both CS remain deselected */
    
    lcd_base[LCD_CTRL_REG_OFFSET/4] = ctrl;
}

void LCD_HW_Backlight(int on)
{
    if (g_fpga_base == NULL) return;
    
    volatile uint32_t *lcd_base = (volatile uint32_t *)
        ((uint8_t *)g_fpga_base + LCD_CONTROLLER_BASE);
    
    uint32_t ctrl = lcd_base[LCD_CTRL_REG_OFFSET/4];
    
    if (on) {
        ctrl |= LCD_CTRL_BACKLIGHT;
    } else {
        ctrl &= ~LCD_CTRL_BACKLIGHT;
    }
    
    lcd_base[LCD_CTRL_REG_OFFSET/4] = ctrl;
}

void LCD_HW_Reset(void)
{
    if (g_fpga_base == NULL) return;
    
    volatile uint32_t *lcd_base = (volatile uint32_t *)
        ((uint8_t *)g_fpga_base + LCD_CONTROLLER_BASE);
    
    /* Assert reset */
    uint32_t ctrl = lcd_base[LCD_CTRL_REG_OFFSET/4];
    ctrl |= LCD_CTRL_RESET;
    lcd_base[LCD_CTRL_REG_OFFSET/4] = ctrl;
    
    LCD_HW_DelayUs(10000);  /* 10ms reset pulse */
    
    /* Deassert reset */
    ctrl &= ~LCD_CTRL_RESET;
    lcd_base[LCD_CTRL_REG_OFFSET/4] = ctrl;
    
    LCD_HW_DelayUs(50000);  /* 50ms recovery */
}

void LCD_HW_DelayUs(uint32_t us)
{
    usleep(us);
}

/*============================================================================
 * FPGA Interface Functions
 *===========================================================================*/

int LCD_HW_FPGA_Ready(void)
{
    if (g_fpga_base == NULL) return 0;
    
    volatile uint32_t *lcd_base = (volatile uint32_t *)
        ((uint8_t *)g_fpga_base + LCD_CONTROLLER_BASE);
    
    return ((lcd_base[LCD_STATUS_REG_OFFSET/4] & LCD_STATUS_BUSY) == 0);
}

void LCD_HW_FPGA_Write(uint32_t reg_offset, uint32_t value)
{
    if (g_fpga_base == NULL) return;
    
    volatile uint32_t *reg = (volatile uint32_t *)
        ((uint8_t *)g_fpga_base + LCD_CONTROLLER_BASE + reg_offset);
    
    *reg = value;
}

uint32_t LCD_HW_FPGA_Read(uint32_t reg_offset)
{
    if (g_fpga_base == NULL) return 0;
    
    volatile uint32_t *reg = (volatile uint32_t *)
        ((uint8_t *)g_fpga_base + LCD_CONTROLLER_BASE + reg_offset);
    
    return *reg;
}