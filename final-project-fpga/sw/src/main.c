/******************************************************************************
 * Smart Queue System Demo
 * For Terasic DE-Series Board with 128x64 Graphic LCD (KS0108 Controller)
 * 
 * FPGA-HPS Co-Design Project
 * 
 * Author: Enhanced for Model Playground Demo
 * Date: 2024
 * 
 * Description:
 *   Professional queue management system demonstration featuring animated
 *   welcome sequence, real-time queue display, statistics, and button control.
 *
 * Hardware Requirements:
 *   - Terasic DE1-SoC or compatible board
 *   - 128x64 Graphic LCD with KS0108 controller
 *   - Optional: Push buttons, LEDs
 *
 ******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>

/* Include existing Terasic LCD libraries */
#include "terasic_lib.h"      // For get_tick_count(), usleep_range()
#include "LCD_Hw.h"           // Hardware abstraction layer
#include "LCD_Driver.h"       // Low-level LCD driver
#include "lcd_graphic.h"      // DRAW_ functions

/*============================================================================
 * CONFIGURATION & CONSTANTS
 *===========================================================================*/

/* LCD Dimensions */
#define LCD_WIDTH           128
#define LCD_HEIGHT          64
#define LCD_PAGES           8
#define LCD_PAGE_HEIGHT     8

/* System Configuration */
#define MAX_QUEUE_SIZE      20
#define MAX_WAITING_DISPLAY 8
#define QUEUE_PREFIX        'A'
#define AUTO_ADVANCE_MS     30000   /* Auto-advance every 30 seconds */
#define ANIMATION_DELAY_MS  50      /* Animation frame delay */
#define BLINK_INTERVAL_MS   500     /* Blinking element interval */

/* Screen States */
typedef enum {
    SCREEN_WELCOME,
    SCREEN_INIT_PROGRESS,
    SCREEN_QUEUE_STATUS,
    SCREEN_STATISTICS,
    SCREEN_TRANSITION
} ScreenState_t;

/* Button Definitions (simulated if hardware not available) */
#define BTN_NEXT_CUSTOMER   0x01
#define BTN_ADD_CUSTOMER    0x02
#define BTN_TOGGLE_SCREEN   0x04
#define BTN_RESET_SYSTEM    0x08

/* Animation Types */
typedef enum {
    ANIM_NONE,
    ANIM_SCROLL_LEFT,
    ANIM_SCROLL_RIGHT,
    ANIM_FADE_IN,
    ANIM_FADE_OUT,
    ANIM_WIPE_DOWN,
    ANIM_WIPE_UP
} AnimationType_t;

/*============================================================================
 * DATA STRUCTURES
 *===========================================================================*/

/* Customer Entry */
typedef struct {
    char prefix;
    uint16_t number;
    uint32_t arrive_time;
    uint8_t priority;       /* 0=normal, 1=priority */
    bool served;
} Customer_t;

/* Queue Statistics */
typedef struct {
    uint16_t total_today;
    uint16_t served_today;
    uint16_t waiting_count;
    float avg_wait_time;    /* in minutes */
    uint16_t hourly_stats[24];
    uint8_t peak_hour;
    uint16_t peak_count;
} QueueStats_t;

/* System Time (simulated) */
typedef struct {
    uint8_t hours;
    uint8_t minutes;
    uint8_t seconds;
    uint32_t last_update_tick;
} SystemTime_t;

/* Display State */
typedef struct {
    ScreenState_t current_screen;
    ScreenState_t previous_screen;
    bool needs_refresh;
    bool blink_state;
    uint32_t last_blink_tick;
    uint8_t animation_frame;
    bool animation_complete;
} DisplayState_t;

/* Queue System State */
typedef struct {
    Customer_t queue[MAX_QUEUE_SIZE];
    uint8_t queue_head;
    uint8_t queue_tail;
    uint16_t current_serving;
    char current_prefix;
    uint16_t next_ticket_number;
    uint32_t last_auto_advance;
    bool system_active;
} QueueSystem_t;

/*============================================================================
 * GLOBAL VARIABLES
 *===========================================================================*/

static QueueSystem_t g_queue;
static QueueStats_t g_stats;
static DisplayState_t g_display;
static SystemTime_t g_time;

/* Simulated LED state (for boards without LEDs) */
static uint8_t g_led_state = 0;

/* Button debounce */
static uint8_t g_last_button_state = 0;
static uint32_t g_button_debounce_tick = 0;
#define DEBOUNCE_DELAY_MS 200

/*============================================================================
 * TERASIC LOGO BITMAP (16x16 simplified)
 *===========================================================================*/

/* Simplified Terasic-style logo bitmap */
static const uint8_t terasic_logo_16x16[] = {
    0x00, 0x00,  /* Row 0 */
    0x7F, 0xFE,  /* Row 1 */
    0x40, 0x02,  /* Row 2 */
    0x5F, 0xFA,  /* Row 3 */
    0x50, 0x0A,  /* Row 4 */
    0x57, 0xEA,  /* Row 5 */
    0x54, 0x2A,  /* Row 6 */
    0x54, 0x2A,  /* Row 7 */
    0x54, 0x2A,  /* Row 8 */
    0x57, 0xEA,  /* Row 9 */
    0x50, 0x0A,  /* Row 10 */
    0x5F, 0xFA,  /* Row 11 */
    0x40, 0x02,  /* Row 12 */
    0x7F, 0xFE,  /* Row 13 */
    0x00, 0x00,  /* Row 14 */
    0x00, 0x00   /* Row 15 */
};

/* Customer icon (8x8) */
static const uint8_t customer_icon_8x8[] = {
    0x3C,  /* ..####.. */
    0x42,  /* .#....#. */
    0x42,  /* .#....#. */
    0x3C,  /* ..####.. */
    0x18,  /* ...##... */
    0x3C,  /* ..####.. */
    0x66,  /* .##..##. */
    0xC3   /* ##....## */
};

/*============================================================================
 * FONT DATA (5x7 for large numbers, using existing if available)
 *===========================================================================*/

/* Large digit font (8x14 for "NOW SERVING" display) */
static const uint8_t large_digits[][14] = {
    /* 0 */ {0x3C,0x7E,0xE7,0xC3,0xC3,0xC3,0xC3,0xC3,0xC3,0xC3,0xE7,0x7E,0x3C,0x00},
    /* 1 */ {0x18,0x38,0x78,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x7E,0x7E,0x00},
    /* 2 */ {0x3C,0x7E,0xE7,0x03,0x03,0x06,0x0C,0x18,0x30,0x60,0xC0,0xFF,0xFF,0x00},
    /* 3 */ {0x3C,0x7E,0xE7,0x03,0x03,0x1E,0x1E,0x03,0x03,0x03,0xE7,0x7E,0x3C,0x00},
    /* 4 */ {0x06,0x0E,0x1E,0x36,0x66,0xC6,0xFF,0xFF,0x06,0x06,0x06,0x06,0x06,0x00},
    /* 5 */ {0xFF,0xFF,0xC0,0xC0,0xFC,0xFE,0x07,0x03,0x03,0x03,0xE7,0x7E,0x3C,0x00},
    /* 6 */ {0x3C,0x7E,0xE0,0xC0,0xFC,0xFE,0xC7,0xC3,0xC3,0xC3,0xE7,0x7E,0x3C,0x00},
    /* 7 */ {0xFF,0xFF,0x03,0x06,0x0C,0x18,0x18,0x30,0x30,0x30,0x30,0x30,0x30,0x00},
    /* 8 */ {0x3C,0x7E,0xE7,0xC3,0xE7,0x3C,0x3C,0xE7,0xC3,0xC3,0xE7,0x7E,0x3C,0x00},
    /* 9 */ {0x3C,0x7E,0xE7,0xC3,0xC3,0xE3,0x7F,0x3F,0x03,0x03,0x07,0x7E,0x3C,0x00}
};

/*============================================================================
 * UTILITY FUNCTIONS
 *===========================================================================*/

/**
 * @brief Get current system tick (wrapper for portability)
 */
static uint32_t SYS_GetTick(void)
{
    return get_tick_count();
}

/**
 * @brief Delay in milliseconds (non-blocking when possible)
 */
static void SYS_DelayMs(uint32_t ms)
{
    usleep_range(ms * 1000, (ms + 1) * 1000);
}

/**
 * @brief Check if specified time has elapsed
 */
static bool SYS_TimeElapsed(uint32_t start_tick, uint32_t duration_ms)
{
    uint32_t current = SYS_GetTick();
    /* Handle tick counter wraparound */
    return ((current - start_tick) >= duration_ms);
}

/**
 * @brief Format queue number as string (e.g., "A101")
 */
static void FormatQueueNumber(char *buf, char prefix, uint16_t number)
{
    sprintf(buf, "%c%03d", prefix, number);
}

/**
 * @brief Format time as string (e.g., "14:30")
 */
static void FormatTime(char *buf, uint8_t hours, uint8_t minutes)
{
    sprintf(buf, "%02d:%02d", hours, minutes);
}

/*============================================================================
 * ENHANCED DRAWING FUNCTIONS (Built on existing DRAW_ functions)
 *===========================================================================*/

/**
 * @brief Draw a bitmap at specified position
 */
static void DRAW_Bitmap(uint8_t x, uint8_t y, uint8_t width, uint8_t height, 
                        const uint8_t *bitmap)
{
    uint8_t bytes_per_row = (width + 7) / 8;
    
    for (uint8_t row = 0; row < height; row++) {
        for (uint8_t col = 0; col < width; col++) {
            uint8_t byte_index = row * bytes_per_row + (col / 8);
            uint8_t bit_index = 7 - (col % 8);
            
            if (bitmap[byte_index] & (1 << bit_index)) {
                DRAW_Pixel(x + col, y + row, 1);
            }
        }
    }
}

/**
 * @brief Draw a large digit (8x14) at specified position
 */
static void DRAW_LargeDigit(uint8_t x, uint8_t y, uint8_t digit)
{
    if (digit > 9) return;
    
    for (uint8_t row = 0; row < 14; row++) {
        uint8_t line = large_digits[digit][row];
        for (uint8_t col = 0; col < 8; col++) {
            if (line & (0x80 >> col)) {
                DRAW_Pixel(x + col, y + row, 1);
            }
        }
    }
}

/**
 * @brief Draw a large character (for queue display)
 */
static void DRAW_LargeChar(uint8_t x, uint8_t y, char c)
{
    if (c >= '0' && c <= '9') {
        DRAW_LargeDigit(x, y, c - '0');
    } else if (c >= 'A' && c <= 'Z') {
        /* Use standard font scaled, or draw custom letter */
        /* For simplicity, use rectangle placeholder */
        DRAW_Rect(x, y, x + 7, y + 13, 1);
        DRAW_Char(x + 1, y + 3, c);  /* Use existing DRAW_Char if available */
    }
}

/**
 * @brief Draw a filled progress bar
 */
static void DRAW_ProgressBar(uint8_t x, uint8_t y, uint8_t width, uint8_t height,
                             uint8_t percentage)
{
    /* Draw border */
    DRAW_Rect(x, y, x + width - 1, y + height - 1, 1);
    
    /* Calculate fill width */
    uint8_t fill_width = ((width - 2) * percentage) / 100;
    
    /* Fill the bar */
    if (fill_width > 0) {
        DRAW_RectFill(x + 1, y + 1, x + fill_width, y + height - 2, 1);
    }
}

/**
 * @brief Draw a vertical bar for charts
 */
static void DRAW_VerticalBar(uint8_t x, uint8_t y_base, uint8_t width, 
                             uint8_t max_height, uint8_t value, uint8_t max_value)
{
    if (max_value == 0) return;
    
    uint8_t bar_height = (max_height * value) / max_value;
    if (bar_height == 0 && value > 0) bar_height = 1;
    
    DRAW_RectFill(x, y_base - bar_height + 1, x + width - 1, y_base, 1);
}

/**
 * @brief Draw horizontal line (with optional pattern for dashed lines)
 */
static void DRAW_HLine(uint8_t x, uint8_t y, uint8_t length, bool dashed)
{
    for (uint8_t i = 0; i < length; i++) {
        if (!dashed || (i % 3 != 2)) {
            DRAW_Pixel(x + i, y, 1);
        }
    }
}

/**
 * @brief Draw vertical line
 */
static void DRAW_VLine(uint8_t x, uint8_t y, uint8_t length, bool dashed)
{
    for (uint8_t i = 0; i < length; i++) {
        if (!dashed || (i % 3 != 2)) {
            DRAW_Pixel(x, y + i, 1);
        }
    }
}

/**
 * @brief Draw customer icon
 */
static void DRAW_CustomerIcon(uint8_t x, uint8_t y)
{
    DRAW_Bitmap(x, y, 8, 8, customer_icon_8x8);
}

/**
 * @brief Draw a frame/border around the screen
 */
static void DRAW_ScreenFrame(void)
{
    DRAW_Rect(0, 0, LCD_WIDTH - 1, LCD_HEIGHT - 1, 1);
}

/**
 * @brief Draw header bar with title
 */
static void DRAW_HeaderBar(const char *title)
{
    /* Draw filled header bar */
    DRAW_RectFill(0, 0, LCD_WIDTH - 1, 9, 1);
    
    /* Calculate centered position */
    uint8_t title_len = strlen(title);
    uint8_t x = (LCD_WIDTH - (title_len * 6)) / 2;
    
    /* Draw title in inverse (white on black) */
    DRAW_String(x, 1, title);  /* Note: May need inverse flag */
    
    /* For inverse text, we need to XOR or use a different approach */
    /* This depends on your existing DRAW_String implementation */
}

/**
 * @brief Draw footer bar with text
 */
static void DRAW_FooterBar(const char *text)
{
    uint8_t y = LCD_HEIGHT - 9;
    DRAW_HLine(0, y - 1, LCD_WIDTH, false);
    DRAW_String(2, y + 1, text);
}

/*============================================================================
 * ANIMATION FUNCTIONS
 *===========================================================================*/

/**
 * @brief Scroll text in from right
 */
static void ANIM_ScrollTextIn(const char *text, uint8_t y, uint8_t final_x)
{
    uint8_t text_width = strlen(text) * 6;
    int16_t x = LCD_WIDTH;  /* Start from right edge */
    
    while (x > final_x) {
        LCD_Clear();
        if (x < LCD_WIDTH) {
            DRAW_String(x, y, text);
        }
        LCD_Update();
        SYS_DelayMs(ANIMATION_DELAY_MS);
        x -= 4;  /* Move 4 pixels per frame */
    }
    
    /* Final position */
    LCD_Clear();
    DRAW_String(final_x, y, text);
    LCD_Update();
}

/**
 * @brief Typewriter effect for text
 */
static void ANIM_TypewriterText(const char *text, uint8_t x, uint8_t y)
{
    uint8_t len = strlen(text);
    char buffer[32];
    
    for (uint8_t i = 0; i <= len; i++) {
        strncpy(buffer, text, i);
        buffer[i] = '\0';
        
        DRAW_String(x, y, buffer);
        LCD_Update();
        SYS_DelayMs(80);
    }
}

/**
 * @brief Progress bar animation
 */
static void ANIM_ProgressBarFill(uint8_t x, uint8_t y, uint8_t width, 
                                  uint8_t height, uint32_t duration_ms)
{
    uint32_t start_tick = SYS_GetTick();
    uint8_t last_percentage = 0;
    
    while (1) {
        uint32_t elapsed = SYS_GetTick() - start_tick;
        uint8_t percentage = (elapsed * 100) / duration_ms;
        
        if (percentage > 100) percentage = 100;
        
        if (percentage != last_percentage) {
            DRAW_ProgressBar(x, y, width, height, percentage);
            LCD_Update();
            last_percentage = percentage;
        }
        
        if (percentage >= 100) break;
        
        SYS_DelayMs(20);
    }
}

/**
 * @brief Screen wipe transition (top to bottom)
 */
static void ANIM_WipeDown(void)
{
    for (uint8_t y = 0; y < LCD_HEIGHT; y += 4) {
        for (uint8_t i = 0; i < 4 && (y + i) < LCD_HEIGHT; i++) {
            for (uint8_t x = 0; x < LCD_WIDTH; x++) {
                DRAW_Pixel(x, y + i, 0);
            }
        }
        LCD_Update();
        SYS_DelayMs(15);
    }
}

/**
 * @brief Fade effect using dithering pattern
 */
static void ANIM_FadeOut(uint8_t steps)
{
    /* Create dithering patterns for fade effect */
    static const uint8_t dither_patterns[] = {
        0xFF, 0xAA, 0x55, 0x22, 0x00
    };
    
    for (uint8_t step = 0; step < steps && step < 5; step++) {
        uint8_t pattern = dither_patterns[step];
        
        for (uint8_t y = 0; y < LCD_HEIGHT; y++) {
            uint8_t row_pattern = (y % 2) ? pattern : (pattern >> 1);
            for (uint8_t x = 0; x < LCD_WIDTH; x++) {
                if (!((row_pattern >> (x % 8)) & 1)) {
                    DRAW_Pixel(x, y, 0);
                }
            }
        }
        LCD_Update();
        SYS_DelayMs(100);
    }
}

/**
 * @brief Blink attention animation
 */
static void ANIM_BlinkArea(uint8_t x, uint8_t y, uint8_t width, uint8_t height,
                           uint8_t blink_count)
{
    for (uint8_t i = 0; i < blink_count * 2; i++) {
        bool fill = (i % 2 == 0);
        DRAW_RectFill(x, y, x + width - 1, y + height - 1, fill);
        LCD_Update();
        SYS_DelayMs(150);
    }
}

/*============================================================================
 * QUEUE MANAGEMENT FUNCTIONS
 *===========================================================================*/

/**
 * @brief Initialize the queue system
 */
static void Queue_Init(void)
{
    memset(&g_queue, 0, sizeof(QueueSystem_t));
    memset(&g_stats, 0, sizeof(QueueStats_t));
    
    g_queue.current_prefix = QUEUE_PREFIX;
    g_queue.current_serving = 100;  /* Start at A100 */
    g_queue.next_ticket_number = 101;
    g_queue.queue_head = 0;
    g_queue.queue_tail = 0;
    g_queue.system_active = true;
    g_queue.last_auto_advance = SYS_GetTick();
    
    /* Initialize with some demo customers */
    for (int i = 0; i < 8; i++) {
        Queue_AddCustomer();
    }
    
    /* Set initial stats */
    g_stats.total_today = 42;
    g_stats.served_today = 34;
    g_stats.waiting_count = 8;
    g_stats.avg_wait_time = 4.5f;
    
    /* Demo hourly data */
    g_stats.hourly_stats[9] = 5;
    g_stats.hourly_stats[10] = 8;
    g_stats.hourly_stats[11] = 6;
    g_stats.hourly_stats[12] = 4;
    g_stats.hourly_stats[13] = 7;
    g_stats.hourly_stats[14] = 12;  /* Peak */
    g_stats.hourly_stats[15] = 9;
    g_stats.hourly_stats[16] = 6;
    g_stats.peak_hour = 14;
    g_stats.peak_count = 12;
}

/**
 * @brief Add a new customer to the queue
 */
static bool Queue_AddCustomer(void)
{
    uint8_t next_tail = (g_queue.queue_tail + 1) % MAX_QUEUE_SIZE;
    
    if (next_tail == g_queue.queue_head) {
        return false;  /* Queue full */
    }
    
    Customer_t *customer = &g_queue.queue[g_queue.queue_tail];
    customer->prefix = g_queue.current_prefix;
    customer->number = g_queue.next_ticket_number++;
    customer->arrive_time = SYS_GetTick();
    customer->priority = 0;
    customer->served = false;
    
    g_queue.queue_tail = next_tail;
    g_stats.waiting_count++;
    g_stats.total_today++;
    
    return true;
}

/**
 * @brief Call next customer
 */
static bool Queue_CallNext(void)
{
    if (g_queue.queue_head == g_queue.queue_tail) {
        return false;  /* Queue empty */
    }
    
    Customer_t *customer = &g_queue.queue[g_queue.queue_head];
    g_queue.current_serving = customer->number;
    customer->served = true;
    
    g_queue.queue_head = (g_queue.queue_head + 1) % MAX_QUEUE_SIZE;
    g_stats.served_today++;
    if (g_stats.waiting_count > 0) g_stats.waiting_count--;
    
    /* Update hourly stats */
    if (g_time.hours < 24) {
        g_stats.hourly_stats[g_time.hours]++;
        if (g_stats.hourly_stats[g_time.hours] > g_stats.peak_count) {
            g_stats.peak_count = g_stats.hourly_stats[g_time.hours];
            g_stats.peak_hour = g_time.hours;
        }
    }
    
    return true;
}

/**
 * @brief Get number of waiting customers
 */
static uint8_t Queue_GetWaitingCount(void)
{
    if (g_queue.queue_tail >= g_queue.queue_head) {
        return g_queue.queue_tail - g_queue.queue_head;
    } else {
        return MAX_QUEUE_SIZE - g_queue.queue_head + g_queue.queue_tail;
    }
}

/**
 * @brief Get customer at position in queue (0 = next to be served)
 */
static Customer_t* Queue_GetCustomerAt(uint8_t position)
{
    uint8_t count = Queue_GetWaitingCount();
    if (position >= count) return NULL;
    
    uint8_t index = (g_queue.queue_head + position) % MAX_QUEUE_SIZE;
    return &g_queue.queue[index];
}

/**
 * @brief Reset the queue system
 */
static void Queue_Reset(void)
{
    Queue_Init();
    g_stats.total_today = 0;
    g_stats.served_today = 0;
    g_stats.waiting_count = 0;
    memset(g_stats.hourly_stats, 0, sizeof(g_stats.hourly_stats));
}

/*============================================================================
 * TIME MANAGEMENT FUNCTIONS
 *===========================================================================*/

/**
 * @brief Initialize system time (simulated)
 */
static void Time_Init(void)
{
    g_time.hours = 9;  /* Start at 9:00 AM */
    g_time.minutes = 0;
    g_time.seconds = 0;
    g_time.last_update_tick = SYS_GetTick();
}

/**
 * @brief Update system time (accelerated for demo)
 */
static void Time_Update(void)
{
    /* In demo mode, 1 real second = 1 simulated minute */
    if (SYS_TimeElapsed(g_time.last_update_tick, 1000)) {
        g_time.last_update_tick = SYS_GetTick();
        
        g_time.minutes++;
        if (g_time.minutes >= 60) {
            g_time.minutes = 0;
            g_time.hours++;
            if (g_time.hours >= 24) {
                g_time.hours = 0;
            }
        }
    }
}

/*============================================================================
 * SCREEN RENDERING FUNCTIONS
 *===========================================================================*/

/**
 * @brief Draw welcome screen with Terasic logo
 */
static void Screen_DrawWelcome(void)
{
    LCD_Clear();
    
    /* Draw border frame */
    DRAW_ScreenFrame();
    
    /* Draw Terasic logo centered at top */
    uint8_t logo_x = (LCD_WIDTH - 16) / 2;
    DRAW_Bitmap(logo_x, 5, 16, 16, terasic_logo_16x16);
    
    /* Draw decorative lines */
    DRAW_HLine(10, 24, LCD_WIDTH - 20, false);
    
    /* Title text */
    DRAW_String(4, 28, ">> SMART QUEUE <<");
    DRAW_String(20, 38, "SYSTEM DEMO");
    
    /* Footer */
    DRAW_HLine(10, 50, LCD_WIDTH - 20, true);
    DRAW_String(4, 54, "FPGA-HPS Co-Design");
    
    LCD_Update();
}

/**
 * @brief Draw initialization progress screen
 */
static void Screen_DrawInitProgress(void)
{
    LCD_Clear();
    
    DRAW_String(20, 10, "INITIALIZING...");
    
    /* System check items */
    DRAW_String(10, 24, "[*] LCD Module");
    DRAW_String(10, 32, "[*] Queue Engine");
    DRAW_String(10, 40, "[*] Time Service");
    
    /* Progress bar at bottom */
    DRAW_String(10, 52, "Loading:");
    ANIM_ProgressBarFill(50, 50, 68, 10, 2000);
    
    SYS_DelayMs(500);
}

/**
 * @brief Draw the main queue status screen
 */
static void Screen_DrawQueueStatus(void)
{
    char buffer[32];
    
    LCD_Clear();
    
    /* Header */
    DRAW_RectFill(0, 0, LCD_WIDTH - 1, 8, 1);
    /* For inverse text, we'll draw white pixels on black background */
    DRAW_String(2, 1, "QUEUE MANAGEMENT");  /* May need inverse handling */
    
    /* Draw clock in header */
    FormatTime(buffer, g_time.hours, g_time.minutes);
    DRAW_String(LCD_WIDTH - 30, 1, buffer);
    
    /* Main serving display */
    DRAW_String(4, 12, "NOW SERVING:");
    
    /* Large queue number display */
    FormatQueueNumber(buffer, g_queue.current_prefix, g_queue.current_serving);
    /* Draw large centered number */
    uint8_t num_x = 40;
    for (int i = 0; buffer[i] != '\0'; i++) {
        if (buffer[i] >= '0' && buffer[i] <= '9') {
            DRAW_LargeDigit(num_x, 20, buffer[i] - '0');
        } else {
            /* Draw letter using rectangle with char inside */
            DRAW_RectFill(num_x, 20, num_x + 7, 33, 1);
            DRAW_Char(num_x + 1, 23, buffer[i]);
        }
        num_x += 10;
    }
    
    /* Next in queue */
    DRAW_String(4, 36, "Next:");
    uint8_t waiting = Queue_GetWaitingCount();
    uint8_t x_pos = 32;
    
    for (uint8_t i = 0; i < 3 && i < waiting; i++) {
        Customer_t *cust = Queue_GetCustomerAt(i);
        if (cust) {
            FormatQueueNumber(buffer, cust->prefix, cust->number);
            DRAW_String(x_pos, 36, buffer);
            x_pos += 30;
        }
    }
    
    /* Visual queue indicators (progress bars showing wait status) */
    DRAW_HLine(0, 46, LCD_WIDTH, false);
    
    uint8_t bar_width = 14;
    uint8_t bar_height = 6;
    uint8_t bar_y = 48;
    
    for (uint8_t i = 0; i < 8; i++) {
        uint8_t bar_x = 2 + i * 16;
        Customer_t *cust = Queue_GetCustomerAt(i);
        
        if (cust) {
            /* Calculate wait percentage (max 10 min = 100%) */
            uint32_t wait_ms = SYS_GetTick() - cust->arrive_time;
            uint8_t wait_pct = (wait_ms / 600000) * 100;  /* 10 min max */
            if (wait_pct > 100) wait_pct = 100;
            
            DRAW_ProgressBar(bar_x, bar_y, bar_width, bar_height, wait_pct);
        } else {
            /* Empty slot */
            DRAW_Rect(bar_x, bar_y, bar_x + bar_width - 1, bar_y + bar_height - 1, 1);
        }
    }
    
    /* Footer with stats */
    sprintf(buffer, "W:%d Avg:%.1fmin", g_stats.waiting_count, g_stats.avg_wait_time);
    DRAW_String(2, 56, buffer);
    
    /* LED indicator simulation (on LCD) */
    DRAW_String(100, 56, g_led_state ? "[*]" : "[ ]");
    
    LCD_Update();
}

/**
 * @brief Draw statistics screen
 */
static void Screen_DrawStatistics(void)
{
    char buffer[32];
    
    LCD_Clear();
    
    /* Header */
    DRAW_RectFill(0, 0, LCD_WIDTH - 1, 8, 1);
    DRAW_String(8, 1, "DAILY STATISTICS");
    
    /* Draw time in corner */
    FormatTime(buffer, g_time.hours, g_time.minutes);
    DRAW_String(LCD_WIDTH - 30, 1, buffer);
    
    /* Bar chart area */
    uint8_t chart_x = 15;
    uint8_t chart_y = 40;  /* Base of bars */
    uint8_t chart_height = 25;
    uint8_t bar_width = 8;
    uint8_t bar_gap = 2;
    
    /* Find max value for scaling */
    uint8_t max_val = 1;
    for (int h = 9; h <= 17; h++) {
        if (g_stats.hourly_stats[h] > max_val) {
            max_val = g_stats.hourly_stats[h];
        }
    }
    
    /* Draw Y axis */
    DRAW_VLine(chart_x - 2, chart_y - chart_height, chart_height + 2, false);
    
    /* Draw X axis */
    DRAW_HLine(chart_x - 2, chart_y + 1, 100, false);
    
    /* Draw bars for hours 9-17 */
    for (int h = 9; h <= 17; h++) {
        uint8_t bar_x = chart_x + (h - 9) * (bar_width + bar_gap);
        uint8_t value = g_stats.hourly_stats[h];
        
        DRAW_VerticalBar(bar_x, chart_y, bar_width, chart_height, value, max_val);
        
        /* Hour label (abbreviated) */
        if ((h - 9) % 2 == 0) {
            sprintf(buffer, "%d", h);
            DRAW_String(bar_x, chart_y + 3, buffer);
        }
    }
    
    /* Statistics summary */
    sprintf(buffer, "Total:%d Srv:%d", g_stats.total_today, g_stats.served_today);
    DRAW_String(2, 48, buffer);
    
    sprintf(buffer, "Peak:%02d:00(%d)", g_stats.peak_hour, g_stats.peak_count);
    DRAW_String(2, 56, buffer);
    
    LCD_Update();
}

/**
 * @brief Draw action feedback overlay
 */
static void Screen_DrawActionFeedback(const char *action, const char *detail)
{
    /* Draw a centered notification box */
    uint8_t box_x = 14;
    uint8_t box_y = 20;
    uint8_t box_w = 100;
    uint8_t box_h = 24;
    
    /* Clear area and draw box */
    DRAW_RectFill(box_x, box_y, box_x + box_w, box_y + box_h, 0);
    DRAW_Rect(box_x, box_y, box_x + box_w, box_y + box_h, 1);
    DRAW_Rect(box_x + 1, box_y + 1, box_x + box_w - 1, box_y + box_h - 1, 1);
    
    /* Draw action text */
    uint8_t action_len = strlen(action);
    uint8_t action_x = box_x + (box_w - action_len * 6) / 2;
    DRAW_String(action_x, box_y + 4, action);
    
    /* Draw detail text */
    uint8_t detail_len = strlen(detail);
    uint8_t detail_x = box_x + (box_w - detail_len * 6) / 2;
    DRAW_String(detail_x, box_y + 14, detail);
    
    LCD_Update();
    SYS_DelayMs(800);
}

/**
 * @brief Draw LED simulation on LCD
 */
static void Screen_DrawLEDStatus(uint8_t led_pattern)
{
    /* Draw 4 virtual LEDs in corner of screen */
    uint8_t led_x = LCD_WIDTH - 20;
    uint8_t led_y = 12;
    
    for (int i = 0; i < 4; i++) {
        uint8_t x = led_x + (i * 5);
        bool on = (led_pattern >> i) & 1;
        
        if (on) {
            DRAW_RectFill(x, led_y, x + 3, led_y + 3, 1);
        } else {
            DRAW_Rect(x, led_y, x + 3, led_y + 3, 1);
        }
    }
}

/*============================================================================
 * BUTTON HANDLING (Simulated if hardware unavailable)
 *===========================================================================*/

/**
 * @brief Read button state (implement based on your hardware)
 */
static uint8_t Button_Read(void)
{
    /* 
     * TODO: Replace with actual button reading code for your board
     * Example for DE1-SoC:
     *   return *((volatile uint8_t*)BUTTON_BASE) & 0x0F;
     * 
     * For simulation without hardware buttons, this returns 0
     * and the system will use auto-advance and keyboard input if available
     */
    
    #ifdef TERASIC_HW_BUTTONS
        /* Read actual hardware buttons */
        return TERASIC_ReadButtons() & 0x0F;
    #else
        /* No hardware buttons - return 0 */
        return 0;
    #endif
}

/**
 * @brief Check for button press with debouncing
 */
static uint8_t Button_GetPressed(void)
{
    uint8_t current_state = Button_Read();
    uint8_t pressed = 0;
    
    /* Check for new button press (edge detection) */
    if (current_state != g_last_button_state) {
        if (SYS_TimeElapsed(g_button_debounce_tick, DEBOUNCE_DELAY_MS)) {
            /* Detect rising edge (button pressed) */
            pressed = current_state & ~g_last_button_state;
            g_last_button_state = current_state;
            g_button_debounce_tick = SYS_GetTick();
        }
    }
    
    return pressed;
}

/**
 * @brief Simulate button press (for demo/testing)
 */
static void Button_Simulate(uint8_t button)
{
    /* This can be called to simulate button presses programmatically */
    char buffer[32];
    
    switch (button) {
        case BTN_NEXT_CUSTOMER:
            if (Queue_CallNext()) {
                FormatQueueNumber(buffer, g_queue.current_prefix, g_queue.current_serving);
                Screen_DrawActionFeedback("CALLING NEXT", buffer);
                g_led_state = 0x01;
            } else {
                Screen_DrawActionFeedback("QUEUE EMPTY", "No customers");
            }
            break;
            
        case BTN_ADD_CUSTOMER:
            if (Queue_AddCustomer()) {
                FormatQueueNumber(buffer, g_queue.current_prefix, 
                                  g_queue.next_ticket_number - 1);
                Screen_DrawActionFeedback("NEW CUSTOMER", buffer);
                g_led_state = 0x02;
            } else {
                Screen_DrawActionFeedback("QUEUE FULL", "Max capacity");
            }
            break;
            
        case BTN_TOGGLE_SCREEN:
            if (g_display.current_screen == SCREEN_QUEUE_STATUS) {
                g_display.current_screen = SCREEN_STATISTICS;
            } else {
                g_display.current_screen = SCREEN_QUEUE_STATUS;
            }
            g_display.needs_refresh = true;
            g_led_state = 0x04;
            break;
            
        case BTN_RESET_SYSTEM:
            Screen_DrawActionFeedback("SYSTEM RESET", "Clearing...");
            Queue_Reset();
            Time_Init();
            g_display.needs_refresh = true;
            g_led_state = 0x08;
            break;
    }
}

/*============================================================================
 * DISPLAY MANAGEMENT
 *===========================================================================*/

/**
 * @brief Initialize display system
 */
static void Display_Init(void)
{
    memset(&g_display, 0, sizeof(DisplayState_t));
    g_display.current_screen = SCREEN_WELCOME;
    g_display.needs_refresh = true;
    g_display.last_blink_tick = SYS_GetTick();
}

/**
 * @brief Update blink state
 */
static void Display_UpdateBlink(void)
{
    if (SYS_TimeElapsed(g_display.last_blink_tick, BLINK_INTERVAL_MS)) {
        g_display.blink_state = !g_display.blink_state;
        g_display.last_blink_tick = SYS_GetTick();
    }
}

/**
 * @brief Transition between screens with animation
 */
static void Display_Transition(ScreenState_t new_screen)
{
    g_display.previous_screen = g_display.current_screen;
    g_display.current_screen = new_screen;
    
    /* Perform transition animation */
    ANIM_WipeDown();
    
    g_display.needs_refresh = true;
}

/**
 * @brief Main display update function
 */
static void Display_Update(void)
{
    Display_UpdateBlink();
    
    if (!g_display.needs_refresh) {
        return;
    }
    
    switch (g_display.current_screen) {
        case SCREEN_WELCOME:
            Screen_DrawWelcome();
            break;
            
        case SCREEN_INIT_PROGRESS:
            Screen_DrawInitProgress();
            break;
            
        case SCREEN_QUEUE_STATUS:
            Screen_DrawQueueStatus();
            break;
            
        case SCREEN_STATISTICS:
            Screen_DrawStatistics();
            break;
            
        default:
            break;
    }
    
    g_display.needs_refresh = false;
}

/*============================================================================
 * WELCOME SEQUENCE
 *===========================================================================*/

/**
 * @brief Run the welcome/startup sequence
 */
static void WelcomeSequence_Run(void)
{
    /* Step 1: Clear and show logo */
    LCD_Clear();
    LCD_Update();
    SYS_DelayMs(300);
    
    /* Step 2: Draw logo with animation */
    uint8_t logo_x = (LCD_WIDTH - 16) / 2;
    DRAW_Bitmap(logo_x, 5, 16, 16, terasic_logo_16x16);
    LCD_Update();
    SYS_DelayMs(500);
    
    /* Step 3: Animate title text scrolling in */
    ANIM_ScrollTextIn(">> SMART QUEUE <<", 28, 4);
    SYS_DelayMs(200);
    
    /* Step 4: Typewriter effect for subtitle */
    ANIM_TypewriterText("SYSTEM DEMO", 20, 38);
    SYS_DelayMs(300);
    
    /* Step 5: Draw decorative elements */
    DRAW_HLine(10, 24, LCD_WIDTH - 20, false);
    DRAW_HLine(10, 50, LCD_WIDTH - 20, true);
    DRAW_String(4, 54, "FPGA-HPS Co-Design");
    LCD_Update();
    SYS_DelayMs(1000);
    
    /* Step 6: Show initialization screen */
    g_display.current_screen = SCREEN_INIT_PROGRESS;
    Screen_DrawInitProgress();
    SYS_DelayMs(500);
    
    /* Step 7: Transition to main screen */
    Display_Transition(SCREEN_QUEUE_STATUS);
}

/*============================================================================
 * AUTO-ADVANCE SIMULATION
 *===========================================================================*/

/**
 * @brief Auto-advance system for demo purposes
 */
static void AutoAdvance_Update(void)
{
    static uint8_t demo_counter = 0;
    
    /* Auto-advance every 30 seconds */
    if (SYS_TimeElapsed(g_queue.last_auto_advance, AUTO_ADVANCE_MS)) {
        g_queue.last_auto_advance = SYS_GetTick();
        
        /* Alternate between calling next and adding customers */
        demo_counter++;
        
        if (demo_counter % 3 == 0) {
            /* Add a new customer */
            Queue_AddCustomer();
        } else {
            /* Call next customer */
            Queue_CallNext();
        }
        
        g_display.needs_refresh = true;
        
        /* Flash LED */
        g_led_state = 0x0F;
    }
    
    /* Reset LED state after brief period */
    if (g_led_state && SYS_TimeElapsed(g_queue.last_auto_advance, 200)) {
        g_led_state = 0;
    }
}

/*============================================================================
 * DEMO MODE FUNCTIONS
 *===========================================================================*/

/**
 * @brief Run automated demo sequence
 */
static void Demo_RunSequence(void)
{
    static uint32_t demo_start = 0;
    static uint8_t demo_step = 0;
    
    if (demo_start == 0) {
        demo_start = SYS_GetTick();
    }
    
    uint32_t elapsed = (SYS_GetTick() - demo_start) / 1000;  /* Seconds */
    
    /* Demo steps based on elapsed time */
    switch (demo_step) {
        case 0:
            if (elapsed >= 5) {
                Button_Simulate(BTN_NEXT_CUSTOMER);
                demo_step++;
            }
            break;
            
        case 1:
            if (elapsed >= 10) {
                Button_Simulate(BTN_ADD_CUSTOMER);
                demo_step++;
            }
            break;
            
        case 2:
            if (elapsed >= 15) {
                Button_Simulate(BTN_TOGGLE_SCREEN);
                demo_step++;
            }
            break;
            
        case 3:
            if (elapsed >= 20) {
                Button_Simulate(BTN_TOGGLE_SCREEN);
                demo_step++;
            }
            break;
            
        case 4:
            if (elapsed >= 25) {
                Button_Simulate(BTN_NEXT_CUSTOMER);
                Button_Simulate(BTN_NEXT_CUSTOMER);
                demo_step++;
            }
            break;
            
        default:
            if (elapsed >= 30) {
                demo_start = SYS_GetTick();
                demo_step = 0;
            }
            break;
    }
}

/*============================================================================
 * MAIN FUNCTION
 *===========================================================================*/

int main(int argc, char *argv[])
{
    uint32_t last_refresh_tick;
    bool demo_mode = true;  /* Set to false if using real buttons */
    
    printf("===========================================\n");
    printf("   Smart Queue System Demo\n");
    printf("   FPGA-HPS Co-Design Project\n");
    printf("   For Terasic DE-Series Board\n");
    printf("===========================================\n\n");
    
    /* ===== INITIALIZATION ===== */
    
    printf("[INIT] Initializing LCD hardware...\n");
    
    /* Initialize LCD hardware */
    if (!LCD_Init()) {
        printf("[ERROR] LCD initialization failed!\n");
        return -1;
    }
    printf("[INIT] LCD initialized successfully\n");
    
    /* Initialize subsystems */
    printf("[INIT] Initializing queue system...\n");
    Queue_Init();
    
    printf("[INIT] Initializing time service...\n");
    Time_Init();
    
    printf("[INIT] Initializing display...\n");
    Display_Init();
    
    printf("[INIT] All systems ready\n\n");
    
    /* ===== WELCOME SEQUENCE ===== */
    
    printf("[START] Running welcome sequence...\n");
    WelcomeSequence_Run();
    printf("[START] Welcome sequence complete\n\n");
    
    /* ===== MAIN LOOP ===== */
    
    printf("[RUN] Entering main loop\n");
    printf("[RUN] Press Ctrl+C to exit\n\n");
    
    if (demo_mode) {
        printf("[DEMO] Running in demo mode - auto-advancing\n");
        printf("[DEMO] Button functions:\n");
        printf("       BTN1: Call next customer\n");
        printf("       BTN2: Add new customer\n");
        printf("       BTN3: Toggle screen\n");
        printf("       BTN4: Reset system\n\n");
    }
    
    last_refresh_tick = SYS_GetTick();
    
    while (1) {
        /* ----- UPDATE TIME ----- */
        Time_Update();
        
        /* ----- CHECK BUTTONS ----- */
        uint8_t buttons = Button_GetPressed();
        
        if (buttons & BTN_NEXT_CUSTOMER) {
            Button_Simulate(BTN_NEXT_CUSTOMER);
            g_display.needs_refresh = true;
        }
        
        if (buttons & BTN_ADD_CUSTOMER) {
            Button_Simulate(BTN_ADD_CUSTOMER);
            g_display.needs_refresh = true;
        }
        
        if (buttons & BTN_TOGGLE_SCREEN) {
            Button_Simulate(BTN_TOGGLE_SCREEN);
        }
        
        if (buttons & BTN_RESET_SYSTEM) {
            Button_Simulate(BTN_RESET_SYSTEM);
        }
        
        /* ----- DEMO MODE ----- */
        if (demo_mode) {
            Demo_RunSequence();
        }
        
        /* ----- AUTO-ADVANCE ----- */
        AutoAdvance_Update();
        
        /* ----- REFRESH DISPLAY ----- */
        /* Update display every 100ms or when needed */
        if (g_display.needs_refresh || 
            SYS_TimeElapsed(last_refresh_tick, 100)) {
            
            Display_Update();
            last_refresh_tick = SYS_GetTick();
        }
        
        /* ----- SMALL DELAY ----- */
        /* Prevent busy-waiting, allow system breathing room */
        SYS_DelayMs(10);
    }
    
    /* Cleanup (if needed) */
    LCD_Clear();
    LCD_Update();
    
    printf("[EXIT] Smart Queue System terminated\n");
    
    return 0;
}

/*============================================================================
 * ADDITIONAL UTILITY FUNCTIONS
 *===========================================================================*/

/**
 * @brief Print system status to console (for debugging)
 */
void Debug_PrintStatus(void)
{
    printf("\n----- System Status -----\n");
    printf("Time: %02d:%02d\n", g_time.hours, g_time.minutes);
    printf("Current Serving: %c%03d\n", g_queue.current_prefix, g_queue.current_serving);
    printf("Waiting: %d\n", g_stats.waiting_count);
    printf("Today Total: %d, Served: %d\n", g_stats.total_today, g_stats.served_today);
    printf("Screen: %d\n", g_display.current_screen);
    printf("-------------------------\n\n");
}

/**
 * @brief Self-test function for LCD
 */
void LCD_SelfTest(void)
{
    printf("[TEST] Running LCD self-test...\n");
    
    /* Test 1: Clear screen */
    LCD_Clear();
    LCD_Update();
    SYS_DelayMs(500);
    
    /* Test 2: Draw border */
    DRAW_Rect(0, 0, LCD_WIDTH-1, LCD_HEIGHT-1, 1);
    LCD_Update();
    SYS_DelayMs(500);
    
    /* Test 3: Draw diagonal lines */
    DRAW_Line(0, 0, LCD_WIDTH-1, LCD_HEIGHT-1, 1);
    DRAW_Line(LCD_WIDTH-1, 0, 0, LCD_HEIGHT-1, 1);
    LCD_Update();
    SYS_DelayMs(500);
    
    /* Test 4: Draw text */
    LCD_Clear();
    DRAW_String(20, 28, "LCD TEST OK");
    LCD_Update();
    SYS_DelayMs(1000);
    
    printf("[TEST] LCD self-test complete\n");
}

/*===========================================================================*/
/* END OF FILE                                                               */
/*===========================================================================*/