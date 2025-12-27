/******************************************************************************
 * LCD_Driver.c
 * 
 * LCD Driver - Mid-level driver for KS0108B LCD
 * Manages frame buffer and display operations
 *
 ******************************************************************************/

#include "LCD_Driver.h"
#include "LCD_Hw.h"
#include "terasic_lib.h"

/*============================================================================
 * Private Variables
 *===========================================================================*/

/* Frame buffer: 128 columns x 8 pages = 1024 bytes */
static uint8_t g_frame_buffer[LCD_WIDTH * LCD_PAGES];

/* Current position */
static uint8_t g_current_x = 0;
static uint8_t g_current_page = 0;

/* Display state */
static bool g_initialized = false;
static bool g_display_on = true;
static bool g_inverted = false;
static uint8_t g_start_line = 0;

/*============================================================================
 * Private Functions
 *===========================================================================*/

/**
 * @brief Select appropriate controller based on X coordinate
 */
static uint8_t LCD_SelectController(uint8_t x)
{
    return (x < LCD_COLS_PER_CTRL) ? LCD_CTRL_LEFT : LCD_CTRL_RIGHT;
}

/**
 * @brief Get column address within controller
 */
static uint8_t LCD_GetColumnAddr(uint8_t x)
{
    return (x < LCD_COLS_PER_CTRL) ? x : (x - LCD_COLS_PER_CTRL);
}

/**
 * @brief Set position on LCD hardware
 */
static void LCD_SetHWPosition(uint8_t controller, uint8_t page, uint8_t col)
{
    LCD_HW_WriteCmd(controller, LCD_CMD_SET_X | (page & 0x07));
    LCD_HW_WriteCmd(controller, LCD_CMD_SET_Y | (col & 0x3F));
}

/*============================================================================
 * Public Functions - Initialization
 *===========================================================================*/

int LCD_Init(void)
{
    if (g_initialized) {
        return 0;
    }
    
    /* Initialize hardware layer */
    if (LCD_HW_Init() != 0) {
        ERROR_PRINT("LCD hardware init failed");
        return -1;
    }
    
    /* Initialize both controllers */
    for (int ctrl = 0; ctrl <= 1; ctrl++) {
        /* Turn on display */
        LCD_HW_WriteCmd(ctrl, LCD_CMD_DISPLAY_ON);
        
        /* Set start line to 0 */
        LCD_HW_WriteCmd(ctrl, LCD_CMD_SET_Z | 0);
        
        /* Set page 0, column 0 */
        LCD_HW_WriteCmd(ctrl, LCD_CMD_SET_X | 0);
        LCD_HW_WriteCmd(ctrl, LCD_CMD_SET_Y | 0);
    }
    
    /* Clear frame buffer */
    memset(g_frame_buffer, 0, sizeof(g_frame_buffer));
    
    /* Clear display */
    LCD_Clear();
    LCD_Update();
    
    /* Turn on backlight */
    LCD_HW_Backlight(1);
    
    g_initialized = true;
    g_display_on = true;
    
    INFO_PRINT("LCD driver initialized (128x64)");
    
    return 0;
}

void LCD_DeInit(void)
{
    if (!g_initialized) {
        return;
    }
    
    LCD_Clear();
    LCD_Update();
    LCD_HW_Backlight(0);
    LCD_HW_DeInit();
    
    g_initialized = false;
}

/*============================================================================
 * Public Functions - Display Control
 *===========================================================================*/

void LCD_Clear(void)
{
    memset(g_frame_buffer, 0, sizeof(g_frame_buffer));
}

void LCD_Fill(uint8_t pattern)
{
    memset(g_frame_buffer, pattern, sizeof(g_frame_buffer));
}

void LCD_Update(void)
{
    if (!g_initialized) return;
    
    /* Update each page */
    for (uint8_t page = 0; page < LCD_PAGES; page++) {
        /* Left controller (columns 0-63) */
        LCD_SetHWPosition(LCD_CTRL_LEFT, page, 0);
        for (uint8_t col = 0; col < LCD_COLS_PER_CTRL; col++) {
            uint8_t data = g_frame_buffer[page * LCD_WIDTH + col];
            if (g_inverted) data = ~data;
            LCD_HW_WriteData(LCD_CTRL_LEFT, data);
        }
        
        /* Right controller (columns 64-127) */
        LCD_SetHWPosition(LCD_CTRL_RIGHT, page, 0);
        for (uint8_t col = 0; col < LCD_COLS_PER_CTRL; col++) {
            uint8_t data = g_frame_buffer[page * LCD_WIDTH + LCD_COLS_PER_CTRL + col];
            if (g_inverted) data = ~data;
            LCD_HW_WriteData(LCD_CTRL_RIGHT, data);
        }
    }
}

void LCD_SetContrast(uint8_t contrast)
{
    /* KS0108B doesn't support software contrast control */
    (void)contrast;
}

void LCD_DisplayOn(int on)
{
    if (!g_initialized) return;
    
    uint8_t cmd = on ? LCD_CMD_DISPLAY_ON : LCD_CMD_DISPLAY_OFF;
    LCD_HW_WriteCmd(LCD_CTRL_LEFT, cmd);
    LCD_HW_WriteCmd(LCD_CTRL_RIGHT, cmd);
    
    g_display_on = (on != 0);
}

void LCD_Invert(int invert)
{
    g_inverted = (invert != 0);
    LCD_Update();  /* Refresh with inverted data */
}

/*============================================================================
 * Public Functions - Position Control
 *===========================================================================*/

void LCD_SetPos(uint8_t x, uint8_t y)
{
    if (x >= LCD_WIDTH) x = LCD_WIDTH - 1;
    if (y >= LCD_HEIGHT) y = LCD_HEIGHT - 1;
    
    g_current_x = x;
    g_current_page = y / LCD_PAGE_HEIGHT;
}

uint8_t LCD_GetPosX(void)
{
    return g_current_x;
}

uint8_t LCD_GetPosY(void)
{
    return g_current_page * LCD_PAGE_HEIGHT;
}

/*============================================================================
 * Public Functions - Data Access
 *===========================================================================*/

void LCD_WriteData(uint8_t data)
{
    if (g_current_x < LCD_WIDTH && g_current_page < LCD_PAGES) {
        g_frame_buffer[g_current_page * LCD_WIDTH + g_current_x] = data;
        g_current_x++;
        if (g_current_x >= LCD_WIDTH) {
            g_current_x = 0;
        }
    }
}

void LCD_WriteDataAt(uint8_t x, uint8_t page, uint8_t data)
{
    if (x < LCD_WIDTH && page < LCD_PAGES) {
        g_frame_buffer[page * LCD_WIDTH + x] = data;
    }
}

uint8_t LCD_ReadDataAt(uint8_t x, uint8_t page)
{
    if (x < LCD_WIDTH && page < LCD_PAGES) {
        return g_frame_buffer[page * LCD_WIDTH + x];
    }
    return 0;
}

/*============================================================================
 * Public Functions - Frame Buffer Access
 *===========================================================================*/

uint8_t *LCD_GetFrameBuffer(void)
{
    return g_frame_buffer;
}

void LCD_WriteToBuffer(uint8_t x, uint8_t page, const uint8_t *data, uint8_t len)
{
    if (page >= LCD_PAGES) return;
    
    for (uint8_t i = 0; i < len && (x + i) < LCD_WIDTH; i++) {
        g_frame_buffer[page * LCD_WIDTH + x + i] = data[i];
    }
}

void LCD_ClearBuffer(void)
{
    memset(g_frame_buffer, 0, sizeof(g_frame_buffer));
}

void LCD_SetBuffer(const uint8_t *buffer)
{
    if (buffer != NULL) {
        memcpy(g_frame_buffer, buffer, sizeof(g_frame_buffer));
    }
}

/*============================================================================
 * Public Functions - Scrolling
 *===========================================================================*/

void LCD_ScrollH(int pixels)
{
    if (pixels == 0) return;
    
    uint8_t temp[LCD_WIDTH];
    
    for (uint8_t page = 0; page < LCD_PAGES; page++) {
        uint8_t *row = &g_frame_buffer[page * LCD_WIDTH];
        
        if (pixels > 0) {
            /* Scroll right */
            memcpy(temp, row + LCD_WIDTH - pixels, pixels);
            memmove(row + pixels, row, LCD_WIDTH - pixels);
            memcpy(row, temp, pixels);
        } else {
            /* Scroll left */
            pixels = -pixels;
            memcpy(temp, row, pixels);
            memmove(row, row + pixels, LCD_WIDTH - pixels);
            memcpy(row + LCD_WIDTH - pixels, temp, pixels);
        }
    }
}

void LCD_ScrollV(int pixels)
{
    if (pixels == 0) return;
    
    /* Vertical scroll requires bit manipulation within pages */
    /* This is a simplified version that scrolls by full pages */
    
    int pages = pixels / LCD_PAGE_HEIGHT;
    if (pages == 0) return;
    
    uint8_t temp[LCD_WIDTH * LCD_PAGES];
    memcpy(temp, g_frame_buffer, sizeof(temp));
    
    for (int page = 0; page < LCD_PAGES; page++) {
        int src_page;
        if (pages > 0) {
            /* Scroll down */
            src_page = page - pages;
        } else {
            /* Scroll up */
            src_page = page - pages;
        }
        
        if (src_page >= 0 && src_page < LCD_PAGES) {
            memcpy(&g_frame_buffer[page * LCD_WIDTH],
                   &temp[src_page * LCD_WIDTH], LCD_WIDTH);
        } else {
            memset(&g_frame_buffer[page * LCD_WIDTH], 0, LCD_WIDTH);
        }
    }
}

void LCD_SetStartLine(uint8_t line)
{
    if (!g_initialized) return;
    if (line >= LCD_HEIGHT) line = 0;
    
    g_start_line = line;
    
    LCD_HW_WriteCmd(LCD_CTRL_LEFT, LCD_CMD_SET_Z | line);
    LCD_HW_WriteCmd(LCD_CTRL_RIGHT, LCD_CMD_SET_Z | line);
}