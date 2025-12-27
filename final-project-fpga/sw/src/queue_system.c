/******************************************************************************
 * queue_system.c
 * 
 * Smart Queue System - Core queue management implementation
 * For FPGA-Based Smart Queue Display System
 *
 ******************************************************************************/

#include "queue_system.h"
#include "lcd_driver.h"
#include "lcd_graphic.h"
#include "LCD_Driver.h"
#include "LCD_Lib.h"
#include "terasic_lib.h"
#include "addresses.h"

/*============================================================================
 * Global System State
 *===========================================================================*/

static SystemState_t g_system;

/*============================================================================
 * Bitmap Data - Icons and Graphics
 *===========================================================================*/

/* Terasic-style logo (16x16) */
static const uint8_t logo_bitmap[32] = {
    0x00, 0x00, 0x7F, 0xFE, 0x40, 0x02, 0x5F, 0xFA,
    0x50, 0x0A, 0x57, 0xEA, 0x54, 0x2A, 0x54, 0x2A,
    0x54, 0x2A, 0x57, 0xEA, 0x50, 0x0A, 0x5F, 0xFA,
    0x40, 0x02, 0x7F, 0xFE, 0x00, 0x00, 0x00, 0x00
};

/* Customer icon (8x8) */
static const uint8_t customer_icon[8] = {
    0x3C, 0x42, 0x42, 0x3C, 0x18, 0x3C, 0x66, 0xC3
};

/* Bell/alert icon (8x8) */
static const uint8_t bell_icon[8] = {
    0x18, 0x3C, 0x3C, 0x3C, 0x7E, 0x7E, 0xFF, 0x18
};

/*============================================================================
 * Private Variables
 *===========================================================================*/

static char g_alert_title[32];
static char g_alert_message[64];
static uint32_t g_alert_start_tick;
static uint32_t g_alert_duration;
static bool g_alert_active = false;

/* Demo mode variables */
static uint32_t g_demo_start_tick = 0;
static uint8_t g_demo_step = 0;

/* Button debounce */
static uint32_t g_last_button_tick = 0;
static uint8_t g_last_button_state = 0;
#define BUTTON_DEBOUNCE_MS  200

/*============================================================================
 * Private Function Prototypes
 *===========================================================================*/

static void Screen_DrawWelcome(void);
static void Screen_DrawInit(void);
static void Screen_DrawQueueStatus(void);
static void Screen_DrawStatistics(void);
static void Screen_DrawAlert(void);
static void Screen_DrawSettings(void);

static void Animation_WipeDown(void);
static void Animation_WipeUp(void);
static void Animation_FadeOut(uint8_t steps);
static void Animation_ScrollTextIn(const char *text, uint8_t y, uint8_t final_x);
static void Animation_TypewriterText(const char *text, uint8_t x, uint8_t y);
static void Animation_ProgressBar(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint32_t duration_ms);
static void Animation_BlinkArea(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t count);

static void DrawLargeQueueNumber(uint8_t x, uint8_t y, char prefix, uint16_t number);
static void DrawWaitingIndicators(uint8_t x, uint8_t y);
static void DrawHourlyChart(uint8_t x, uint8_t y, uint8_t width, uint8_t height);
static void DrawClock(uint8_t x, uint8_t y);
static void DrawLEDIndicators(uint8_t x, uint8_t y);

static void UpdateAutoAdvance(void);
static void UpdateBlinkState(void);

/*============================================================================
 * System Control Functions
 *===========================================================================*/

int QueueSystem_Init(void)
{
    INFO_PRINT("========================================");
    INFO_PRINT("  Smart Queue System Initialization");
    INFO_PRINT("  FPGA-HPS Co-Design Project");
    INFO_PRINT("========================================");
    
    /* Clear system state */
    memset(&g_system, 0, sizeof(SystemState_t));
    
    /* Initialize memory mapping */
    INFO_PRINT("[INIT] Opening memory mapping...");
    g_system.fpga_lw_base = TERASIC_MEM_Open();
    if (g_system.fpga_lw_base == NULL) {
        ERROR_PRINT("Failed to open memory mapping");
        return -1;
    }
    INFO_PRINT("[INIT] Memory mapping successful");
    
    /* Initialize LCD */
    INFO_PRINT("[INIT] Initializing LCD display...");
    if (lcd_init() != 0) {
        ERROR_PRINT("Failed to initialize LCD");
        TERASIC_MEM_Close(g_system.fpga_lw_base);
        return -1;
    }
    INFO_PRINT("[INIT] LCD initialized (128x64 KS0108B)");
    
    /* Initialize GPIO for buttons/LEDs */
    INFO_PRINT("[INIT] Initializing GPIO...");
    TERASIC_GPIO_Init();
    TERASIC_LED_Set(0);
    TERASIC_HEX_Clear();
    INFO_PRINT("[INIT] GPIO initialized");
    
    /* Initialize queue state */
    INFO_PRINT("[INIT] Initializing queue engine...");
    g_system.queue.current_prefix = QUEUE_PREFIX_DEFAULT;
    g_system.queue.current_number = QUEUE_START_NUMBER;
    g_system.queue.next_ticket = QUEUE_START_NUMBER + 1;
    g_system.queue.head = 0;
    g_system.queue.tail = 0;
    g_system.queue.count = 0;
    g_system.queue.is_active = true;
    g_system.queue.is_paused = false;
    g_system.queue.auto_advance_enabled = true;
    g_system.queue.last_advance_tick = get_tick_count();
    g_system.queue.service_start_tick = get_tick_count();
    INFO_PRINT("[INIT] Queue engine ready");
    
    /* Initialize display state */
    g_system.display.current_screen = SCREEN_BOOT;
    g_system.display.previous_screen = SCREEN_BOOT;
    g_system.display.needs_refresh = true;
    g_system.display.is_animating = false;
    g_system.display.animation_frame = 0;
    g_system.display.blink_state = false;
    g_system.display.last_blink_tick = get_tick_count();
    g_system.display.backlight_on = true;
    g_system.display.brightness = 100;
    
    /* Initialize time (simulated mode for demo) */
    INFO_PRINT("[INIT] Initializing time service...");
    Time_Init(true);  /* true = simulated/accelerated time */
    g_system.time.hours = 9;
    g_system.time.minutes = 0;
    g_system.time.seconds = 0;
    INFO_PRINT("[INIT] Time service ready (simulated mode)");
    
    /* Reset statistics */
    Stats_ResetDaily();
    
    /* Add initial demo customers */
    INFO_PRINT("[INIT] Adding demo customers...");
    for (int i = 0; i < 5; i++) {
        Queue_AddCustomer(PRIORITY_NORMAL);
    }
    
    /* Set demo statistics for display */
    g_system.stats.total_today = 42;
    g_system.stats.served_today = 37;
    g_system.stats.waiting_count = 5;
    g_system.stats.avg_wait_time = 4.5f;
    g_system.stats.avg_service_time = 3.2f;
    
    /* Hourly distribution data */
    g_system.stats.hourly_customers[9] = 5;
    g_system.stats.hourly_customers[10] = 8;
    g_system.stats.hourly_customers[11] = 6;
    g_system.stats.hourly_customers[12] = 4;
    g_system.stats.hourly_customers[13] = 7;
    g_system.stats.hourly_customers[14] = 12;
    g_system.stats.hourly_customers[15] = 9;
    g_system.stats.hourly_customers[16] = 6;
    g_system.stats.hourly_customers[17] = 3;
    
    g_system.stats.peak_hour = 14;
    g_system.stats.peak_count = 12;
    
    /* Set system flags */
    g_system.initialized = true;
    g_system.running = true;
    g_system.demo_mode = FEATURE_DEMO_MODE;
    
    INFO_PRINT("[INIT] System initialization complete");
    INFO_PRINT("========================================\n");
    
    return 0;
}

void QueueSystem_DeInit(void)
{
    INFO_PRINT("Shutting down Smart Queue System...");
    
    if (!g_system.initialized) {
        return;
    }
    
    g_system.running = false;
    
    /* Clear display */
    lcd_clear();
    lcd_update();
    
    /* Turn off LEDs and 7-segment */
    TERASIC_LED_Set(0);
    TERASIC_HEX_Clear();
    
    /* Deinitialize LCD */
    lcd_deinit();
    
    /* Close memory mapping */
    if (g_system.fpga_lw_base != NULL) {
        TERASIC_MEM_Close(g_system.fpga_lw_base);
        g_system.fpga_lw_base = NULL;
    }
    
    g_system.initialized = false;
    INFO_PRINT("System shutdown complete");
}

void QueueSystem_Update(void)
{
    if (!g_system.initialized || !g_system.running) {
        return;
    }
    
    /* Update time */
    Time_Update();
    
    /* Update blink state for animations */
    UpdateBlinkState();
    
    /* Handle auto-advance if enabled */
    if (g_system.queue.auto_advance_enabled && !g_system.queue.is_paused) {
        UpdateAutoAdvance();
    }
    
    /* Update demo mode if active */
    if (g_system.demo_mode) {
        Demo_Update();
    }
    
    /* Check for alert timeout */
    if (g_alert_active && g_alert_duration > 0) {
        if (get_tick_count() - g_alert_start_tick >= g_alert_duration) {
            Display_DismissAlert();
        }
    }
    
    /* Update display */
    Display_Update();
    
    /* Update hardware indicators */
    TERASIC_LED_Set(g_system.led_pattern);
    TERASIC_HEX_Display(g_system.queue.current_number, 4);
}

void QueueSystem_Reset(void)
{
    INFO_PRINT("Resetting queue system...");
    
    /* Reset queue */
    g_system.queue.current_number = QUEUE_START_NUMBER;
    g_system.queue.next_ticket = QUEUE_START_NUMBER + 1;
    g_system.queue.head = 0;
    g_system.queue.tail = 0;
    g_system.queue.count = 0;
    g_system.queue.last_advance_tick = get_tick_count();
    
    /* Clear customer data */
    memset(g_system.queue.customers, 0, sizeof(g_system.queue.customers));
    
    /* Reset statistics */
    Stats_ResetDaily();
    
    /* Reset time to morning */
    g_system.time.hours = 9;
    g_system.time.minutes = 0;
    g_system.time.seconds = 0;
    
    /* Update display */
    g_system.display.needs_refresh = true;
    
    /* Visual feedback */
    TERASIC_LED_Set(0xFF);
    msleep(200);
    TERASIC_LED_Set(0);
    
    INFO_PRINT("Queue system reset complete");
}

void QueueSystem_Start(void)
{
    g_system.running = true;
    g_system.queue.is_active = true;
    g_system.queue.is_paused = false;
    g_system.queue.last_advance_tick = get_tick_count();
    INFO_PRINT("Queue system started");
}

void QueueSystem_Stop(void)
{
    g_system.running = false;
    g_system.queue.is_active = false;
    INFO_PRINT("Queue system stopped");
}

void QueueSystem_Pause(int pause)
{
    g_system.queue.is_paused = (pause != 0);
    if (!g_system.queue.is_paused) {
        g_system.queue.last_advance_tick = get_tick_count();
    }
    INFO_PRINT("Queue system %s", pause ? "paused" : "resumed");
}

/*============================================================================
 * Queue Operations
 *===========================================================================*/

uint16_t Queue_AddCustomer(CustomerPriority_t priority)
{
    if (Queue_IsFull()) {
        WARNING_PRINT("Queue is full, cannot add customer");
        return 0;
    }
    
    /* Create new customer entry */
    Customer_t *customer = &g_system.queue.customers[g_system.queue.tail];
    customer->prefix = g_system.queue.current_prefix;
    customer->number = g_system.queue.next_ticket;
    customer->arrive_time = get_tick_count();
    customer->serve_time = 0;
    customer->priority = priority;
    customer->service_type = 0;
    customer->served = false;
    customer->no_show = false;
    
    /* Update queue pointers */
    g_system.queue.tail = (g_system.queue.tail + 1) % QUEUE_MAX_SIZE;
    g_system.queue.count++;
    g_system.queue.next_ticket++;
    
    /* Update statistics */
    g_system.stats.total_today++;
    g_system.stats.waiting_count = g_system.queue.count;
    
    /* Update hourly stats */
    if (g_system.time.hours < 24) {
        g_system.stats.hourly_customers[g_system.time.hours]++;
    }
    
    /* Update display */
    g_system.display.needs_refresh = true;
    
    /* LED feedback */
    g_system.led_pattern |= 0x02;
    
    DEBUG_PRINT("Added customer %c%03d (priority=%d, waiting=%d)",
                customer->prefix, customer->number, priority, g_system.queue.count);
    
    return customer->number;
}

bool Queue_CallNext(void)
{
    if (Queue_IsEmpty()) {
        WARNING_PRINT("Queue is empty, no customer to call");
        return false;
    }
    
    /* Get next customer */
    Customer_t *customer = &g_system.queue.customers[g_system.queue.head];
    
    /* Calculate wait time */
    uint32_t wait_time = get_tick_count() - customer->arrive_time;
    
    /* Update current serving */
    g_system.queue.current_number = customer->number;
    customer->serve_time = get_tick_count();
    customer->served = true;
    
    /* Move head pointer */
    g_system.queue.head = (g_system.queue.head + 1) % QUEUE_MAX_SIZE;
    g_system.queue.count--;
    
    /* Record service start time */
    g_system.queue.service_start_tick = get_tick_count();
    g_system.queue.last_advance_tick = get_tick_count();
    
    /* Update statistics */
    g_system.stats.served_today++;
    g_system.stats.waiting_count = g_system.queue.count;
    Stats_UpdateService(wait_time, 0);
    
    /* Update hourly served count */
    if (g_system.time.hours < 24) {
        g_system.stats.hourly_served[g_system.time.hours]++;
        
        /* Check for new peak */
        if (g_system.stats.hourly_served[g_system.time.hours] > g_system.stats.peak_count) {
            g_system.stats.peak_count = g_system.stats.hourly_served[g_system.time.hours];
            g_system.stats.peak_hour = g_system.time.hours;
        }
    }
    
    /* Update display */
    g_system.display.needs_refresh = true;
    
    /* LED feedback */
    g_system.led_pattern |= 0x01;
    
    INFO_PRINT("Called customer %c%03d (wait=%.1fs, remaining=%d)",
               g_system.queue.current_prefix, g_system.queue.current_number,
               wait_time / 1000.0f, g_system.queue.count);
    
    return true;
}

void Queue_MarkServed(void)
{
    /* Calculate service time */
    uint32_t service_time = get_tick_count() - g_system.queue.service_start_tick;
    
    /* Update statistics */
    g_system.stats.total_service_time += service_time;
    g_system.stats.avg_service_time = 
        (float)g_system.stats.total_service_time / 
        (g_system.stats.served_today * 60000.0f);  /* Convert to minutes */
    
    DEBUG_PRINT("Customer served (service_time=%.1fs)", service_time / 1000.0f);
}

void Queue_MarkNoShow(void)
{
    g_system.stats.no_shows_today++;
    WARNING_PRINT("Customer marked as no-show");
}

uint8_t Queue_GetWaitingCount(void)
{
    return g_system.queue.count;
}

Customer_t *Queue_GetCustomerAt(uint8_t position)
{
    if (position >= g_system.queue.count) {
        return NULL;
    }
    
    uint8_t index = (g_system.queue.head + position) % QUEUE_MAX_SIZE;
    return &g_system.queue.customers[index];
}

void Queue_GetCurrentServing(char *prefix, uint16_t *number)
{
    if (prefix != NULL) {
        *prefix = g_system.queue.current_prefix;
    }
    if (number != NULL) {
        *number = g_system.queue.current_number;
    }
}

void Queue_FormatTicket(char *buffer, char prefix, uint16_t number)
{
    if (buffer != NULL) {
        sprintf(buffer, "%c%03d", prefix, number);
    }
}

bool Queue_IsEmpty(void)
{
    return (g_system.queue.count == 0);
}

bool Queue_IsFull(void)
{
    return (g_system.queue.count >= QUEUE_MAX_SIZE);
}

/*============================================================================
 * Statistics Functions
 *===========================================================================*/

QueueStats_t *Stats_Get(void)
{
    return &g_system.stats;
}

void Stats_ResetDaily(void)
{
    memset(&g_system.stats, 0, sizeof(QueueStats_t));
    g_system.stats.shortest_wait = 0xFFFF;  /* Initialize to max */
    INFO_PRINT("Daily statistics reset");
}

void Stats_UpdateService(uint32_t wait_time, uint32_t service_time)
{
    /* Update total wait time */
    g_system.stats.total_wait_time += wait_time;
    
    /* Update average wait time (in minutes) */
    if (g_system.stats.served_today > 0) {
        g_system.stats.avg_wait_time = 
            (float)g_system.stats.total_wait_time / 
            (g_system.stats.served_today * 60000.0f);
    }
    
    /* Update min/max wait times */
    uint16_t wait_minutes = wait_time / 60000;
    if (wait_minutes > g_system.stats.longest_wait) {
        g_system.stats.longest_wait = wait_minutes;
    }
    if (wait_minutes < g_system.stats.shortest_wait) {
        g_system.stats.shortest_wait = wait_minutes;
    }
    
    /* Update service time if provided */
    if (service_time > 0) {
        g_system.stats.total_service_time += service_time;
        g_system.stats.avg_service_time = 
            (float)g_system.stats.total_service_time / 
            (g_system.stats.served_today * 60000.0f);
    }
}

float Stats_GetAvgWaitTime(void)
{
    return g_system.stats.avg_wait_time;
}

/*============================================================================
 * Display Functions
 *===========================================================================*/

void Display_SetScreen(ScreenState_t screen)
{
    if (screen == g_system.display.current_screen) {
        return;
    }
    
    g_system.display.previous_screen = g_system.display.current_screen;
    g_system.display.current_screen = screen;
    g_system.display.needs_refresh = true;
    g_system.display.animation_frame = 0;
    
    DEBUG_PRINT("Screen changed: %d -> %d", 
                g_system.display.previous_screen, screen);
}

ScreenState_t Display_GetScreen(void)
{
    return g_system.display.current_screen;
}

void Display_Refresh(void)
{
    g_system.display.needs_refresh = true;
}

void Display_Update(void)
{
    if (!g_system.display.needs_refresh) {
        return;
    }
    
    /* Handle alert overlay first */
    if (g_alert_active) {
        Screen_DrawAlert();
        lcd_update();
        g_system.display.needs_refresh = false;
        return;
    }
    
    /* Draw appropriate screen */
    switch (g_system.display.current_screen) {
        case SCREEN_BOOT:
        case SCREEN_WELCOME:
            Screen_DrawWelcome();
            break;
            
        case SCREEN_INIT:
            Screen_DrawInit();
            break;
            
        case SCREEN_QUEUE_STATUS:
            Screen_DrawQueueStatus();
            break;
            
        case SCREEN_STATISTICS:
            Screen_DrawStatistics();
            break;
            
        case SCREEN_SETTINGS:
            Screen_DrawSettings();
            break;
            
        default:
            Screen_DrawQueueStatus();
            break;
    }
    
    lcd_update();
    g_system.display.needs_refresh = false;
}

void Display_ShowAlert(const char *title, const char *message, uint32_t duration_ms)
{
    if (title != NULL) {
        strncpy(g_alert_title, title, sizeof(g_alert_title) - 1);
        g_alert_title[sizeof(g_alert_title) - 1] = '\0';
    } else {
        g_alert_title[0] = '\0';
    }
    
    if (message != NULL) {
        strncpy(g_alert_message, message, sizeof(g_alert_message) - 1);
        g_alert_message[sizeof(g_alert_message) - 1] = '\0';
    } else {
        g_alert_message[0] = '\0';
    }
    
    g_alert_start_tick = get_tick_count();
    g_alert_duration = duration_ms;
    g_alert_active = true;
    g_system.display.needs_refresh = true;
}

void Display_DismissAlert(void)
{
    g_alert_active = false;
    g_system.display.needs_refresh = true;
}

/*============================================================================
 * Screen Drawing Functions
 *===========================================================================*/

static void Screen_DrawWelcome(void)
{
    lcd_clear();
    
    /* Draw border */
    DRAW_Rect(0, 0, 127, 63, 1);
    
    /* Draw logo */
    uint8_t logo_x = (128 - 16) / 2;
    DRAW_Bitmap(logo_x, 4, 16, 16, logo_bitmap);
    
    /* Draw decorative line */
    DRAW_Line(10, 23, 117, 23, 1);
    
    /* Draw title */
    DRAW_StringCenter(27, "SMART QUEUE");
    DRAW_StringCenter(37, "SYSTEM");
    
    /* Draw subtitle */
    DRAW_Line(10, 48, 117, 48, 1);
    DRAW_StringCenter(52, "FPGA-HPS Co-Design");
}

static void Screen_DrawInit(void)
{
    lcd_clear();
    
    DRAW_StringCenter(5, "INITIALIZING...");
    
    /* Draw checklist */
    DRAW_String(10, 18, "[*] LCD Module");
    DRAW_String(10, 28, "[*] Queue Engine");
    DRAW_String(10, 38, "[*] Time Service");
    
    /* Draw progress bar */
    DRAW_String(10, 52, "Loading:");
    DRAW_ProgressBar(55, 50, 65, 10, g_system.display.animation_frame);
}

static void Screen_DrawQueueStatus(void)
{
    char buffer[32];
    
    lcd_clear();
    
    /* === Header === */
    DRAW_RectFill(0, 0, 127, 9, 1);
    
    /* Title (inverted) - we'll draw directly since invert is complex */
    LCD_PutString(4, 1, "QUEUE SYSTEM");
    
    /* Clock in header */
    Time_Format(buffer);
    LCD_PutString(98, 1, buffer);
    
    /* === Now Serving Section === */
    DRAW_String(4, 12, "NOW SERVING:");
    
    /* Large queue number */
    DrawLargeQueueNumber(35, 20, g_system.queue.current_prefix, 
                         g_system.queue.current_number);
    
    /* Blinking indicator when active */
    if (g_system.display.blink_state && !Queue_IsEmpty()) {
        DRAW_RectFill(100, 20, 110, 30, 1);
    }
    
    /* === Next in Queue === */
    DRAW_String(4, 36, "Next:");
    uint8_t x_pos = 35;
    uint8_t waiting = Queue_GetWaitingCount();
    
    for (uint8_t i = 0; i < 3 && i < waiting; i++) {
        Customer_t *cust = Queue_GetCustomerAt(i);
        if (cust != NULL) {
            Queue_FormatTicket(buffer, cust->prefix, cust->number);
            DRAW_String(x_pos, 36, buffer);
            x_pos += 32;
        }
    }
    
    if (waiting > 3) {
        sprintf(buffer, "+%d", waiting - 3);
        DRAW_String(x_pos, 36, buffer);
    }
    
    /* === Waiting Indicators === */
    DRAW_Line(0, 45, 127, 45, 1);
    DrawWaitingIndicators(2, 47);
    
    /* === Footer === */
    DRAW_Line(0, 55, 127, 55, 1);
    sprintf(buffer, "Wait:%d Avg:%.1fm", g_system.stats.waiting_count, 
            g_system.stats.avg_wait_time);
    DRAW_String(2, 57, buffer);
    
    /* LED indicator on LCD */
    DrawLEDIndicators(105, 57);
}

static void Screen_DrawStatistics(void)
{
    char buffer[32];
    
    lcd_clear();
    
    /* === Header === */
    DRAW_RectFill(0, 0, 127, 9, 1);
    LCD_PutString(8, 1, "STATISTICS");
    
    /* Clock */
    Time_Format(buffer);
    LCD_PutString(98, 1, buffer);
    
    /* === Bar Chart === */
    DrawHourlyChart(15, 42, 100, 25);
    
    /* === Summary Stats === */
    sprintf(buffer, "Total:%d Srv:%d", 
            g_system.stats.total_today, g_system.stats.served_today);
    DRAW_String(2, 48, buffer);
    
    sprintf(buffer, "Peak:%02d:00 (%d)", 
            g_system.stats.peak_hour, g_system.stats.peak_count);
    DRAW_String(2, 57, buffer);
    
    /* Current time */
    sprintf(buffer, "Now:%02d:%02d", g_system.time.hours, g_system.time.minutes);
    DRAW_String(80, 57, buffer);
}

static void Screen_DrawAlert(void)
{
    /* Draw alert dialog box over current screen */
    uint8_t box_x = 10;
    uint8_t box_y = 15;
    uint8_t box_w = 108;
    uint8_t box_h = 35;
    
    /* Clear area and draw box */
    DRAW_RectFill(box_x, box_y, box_x + box_w - 1, box_y + box_h - 1, 0);
    DRAW_Rect(box_x, box_y, box_x + box_w - 1, box_y + box_h - 1, 1);
    DRAW_Rect(box_x + 2, box_y + 2, box_x + box_w - 3, box_y + box_h - 3, 1);
    
    /* Draw title */
    if (g_alert_title[0] != '\0') {
        uint8_t title_width = strlen(g_alert_title) * 6;
        uint8_t title_x = box_x + (box_w - title_width) / 2;
        DRAW_String(title_x, box_y + 5, g_alert_title);
        
        /* Separator line */
        DRAW_Line(box_x + 5, box_y + 13, box_x + box_w - 6, box_y + 13, 1);
    }
    
    /* Draw message */
    if (g_alert_message[0] != '\0') {
        uint8_t msg_width = strlen(g_alert_message) * 6;
        uint8_t msg_x = box_x + (box_w - msg_width) / 2;
        uint8_t msg_y = (g_alert_title[0] != '\0') ? box_y + 18 : box_y + 12;
        DRAW_String(msg_x, msg_y, g_alert_message);
    }
}

static void Screen_DrawSettings(void)
{
    lcd_clear();
    
    DRAW_RectFill(0, 0, 127, 9, 1);
    LCD_PutString(20, 1, "SETTINGS");
    
    DRAW_String(5, 15, "Auto-advance:");
    DRAW_String(85, 15, g_system.queue.auto_advance_enabled ? "ON" : "OFF");
    
    DRAW_String(5, 25, "Demo mode:");
    DRAW_String(85, 25, g_system.demo_mode ? "ON" : "OFF");
    
    DRAW_String(5, 35, "Backlight:");
    DRAW_String(85, 35, g_system.display.backlight_on ? "ON" : "OFF");
    
    DRAW_String(5, 50, "Press KEY3 to exit");
}

/*============================================================================
 * Drawing Helper Functions
 *===========================================================================*/

static void DrawLargeQueueNumber(uint8_t x, uint8_t y, char prefix, uint16_t number)
{
    char buffer[8];
    Queue_FormatTicket(buffer, prefix, number);
    
    /* Draw each character large */
    uint8_t char_x = x;
    for (int i = 0; buffer[i] != '\0'; i++) {
        LCD_PutLargeChar(char_x, y, buffer[i]);
        char_x += 10;  /* 8 pixels + 2 spacing */
    }
}

static void DrawWaitingIndicators(uint8_t x, uint8_t y)
{
    uint8_t waiting = Queue_GetWaitingCount();
    uint8_t bar_width = 14;
    uint8_t bar_height = 6;
    
    for (uint8_t i = 0; i < 8; i++) {
        uint8_t bar_x = x + i * 16;
        
        if (i < waiting) {
            /* Calculate fill based on wait time */
            Customer_t *cust = Queue_GetCustomerAt(i);
            uint8_t fill_pct = 100;
            
            if (cust != NULL) {
                uint32_t wait_ms = get_tick_count() - cust->arrive_time;
                fill_pct = (wait_ms / 600000.0f) * 100;  /* 10 min = 100% */
                if (fill_pct > 100) fill_pct = 100;
            }
            
            DRAW_ProgressBar(bar_x, y, bar_width, bar_height, fill_pct);
        } else {
            /* Empty slot - just outline */
            DRAW_Rect(bar_x, y, bar_x + bar_width - 1, y + bar_height - 1, 1);
        }
    }
}

static void DrawHourlyChart(uint8_t x, uint8_t y_base, uint8_t width, uint8_t height)
{
    /* Find max value for scaling */
    uint8_t max_val = 1;
    for (int h = 9; h <= 17; h++) {
        if (g_system.stats.hourly_customers[h] > max_val) {
            max_val = g_system.stats.hourly_customers[h];
        }
    }
    
    /* Draw axes */
    DRAW_Line(x - 2, y_base - height, x - 2, y_base + 1, 1);  /* Y axis */
    DRAW_Line(x - 2, y_base + 1, x + width, y_base + 1, 1);   /* X axis */
    
    /* Draw bars for hours 9-17 */
    uint8_t bar_width = 8;
    uint8_t bar_gap = 3;
    
    for (int h = 9; h <= 17; h++) {
        uint8_t bar_x = x + (h - 9) * (bar_width + bar_gap);
        uint8_t value = g_system.stats.hourly_customers[h];
        
        DRAW_VerticalBar(bar_x, y_base, bar_width, height, value, max_val);
    }
}

static void DrawClock(uint8_t x, uint8_t y)
{
    char buffer[8];
    sprintf(buffer, "%02d:%02d", g_system.time.hours, g_system.time.minutes);
    DRAW_String(x, y, buffer);
}

static void DrawLEDIndicators(uint8_t x, uint8_t y)
{
    /* Draw 4 virtual LEDs */
    for (int i = 0; i < 4; i++) {
        uint8_t led_x = x + i * 5;
        bool on = (g_system.led_pattern >> i) & 1;
        
        if (on) {
            DRAW_RectFill(led_x, y, led_x + 3, y + 4, 1);
        } else {
            DRAW_Rect(led_x, y, led_x + 3, y + 4, 1);
        }
    }
}

/*============================================================================
 * Animation Functions
 *===========================================================================*/

static void Animation_WipeDown(void)
{
    for (uint8_t y = 0; y < 64; y += 4) {
        for (uint8_t row = 0; row < 4 && (y + row) < 64; row++) {
            for (uint8_t x = 0; x < 128; x++) {
                DRAW_Pixel(x, y + row, 0);
            }
        }
        lcd_update();
        msleep(15);
    }
}

static void Animation_WipeUp(void)
{
    for (int y = 63; y >= 0; y -= 4) {
        for (int row = 0; row < 4 && (y - row) >= 0; row++) {
            for (uint8_t x = 0; x < 128; x++) {
                DRAW_Pixel(x, y - row, 0);
            }
        }
        lcd_update();
        msleep(15);
    }
}

static void Animation_FadeOut(uint8_t steps)
{
    static const uint8_t dither_patterns[] = {
        0xFF, 0xAA, 0x55, 0x22, 0x00
    };
    
    for (uint8_t step = 0; step < steps && step < 5; step++) {
        uint8_t pattern = dither_patterns[step];
        
        for (uint8_t y = 0; y < 64; y++) {
            uint8_t row_pattern = (y % 2) ? pattern : (pattern >> 1);
            for (uint8_t x = 0; x < 128; x++) {
                if (!((row_pattern >> (x % 8)) & 1)) {
                    DRAW_Pixel(x, y, 0);
                }
            }
        }
        lcd_update();
        msleep(100);
    }
}

static void Animation_ScrollTextIn(const char *text, uint8_t y, uint8_t final_x)
{
    uint8_t text_width = strlen(text) * 6;
    int16_t x = 128;
    
    while (x > (int16_t)final_x) {
        lcd_clear();
        if (x < 128) {
            DRAW_String(x, y, text);
        }
        lcd_update();
        msleep(30);
        x -= 4;
    }
    
    lcd_clear();
    DRAW_String(final_x, y, text);
    lcd_update();
}

static void Animation_TypewriterText(const char *text, uint8_t x, uint8_t y)
{
    char buffer[64];
    uint8_t len = strlen(text);
    
    for (uint8_t i = 0; i <= len; i++) {
        strncpy(buffer, text, i);
        buffer[i] = '\0';
        
        DRAW_String(x, y, buffer);
        lcd_update();
        msleep(60);
    }
}

static void Animation_ProgressBar(uint8_t x, uint8_t y, uint8_t w, uint8_t h, 
                                   uint32_t duration_ms)
{
    uint32_t start_tick = get_tick_count();
    uint8_t last_pct = 0;
    
    while (1) {
        uint32_t elapsed = get_tick_count() - start_tick;
        uint8_t pct = (elapsed * 100) / duration_ms;
        if (pct > 100) pct = 100;
        
        if (pct != last_pct) {
            DRAW_ProgressBar(x, y, w, h, pct);
            lcd_update();
            last_pct = pct;
        }
        
        if (pct >= 100) break;
        msleep(20);
    }
}

static void Animation_BlinkArea(uint8_t x, uint8_t y, uint8_t w, uint8_t h, 
                                 uint8_t count)
{
    for (uint8_t i = 0; i < count * 2; i++) {
        bool fill = (i % 2 == 0);
        DRAW_RectFill(x, y, x + w - 1, y + h - 1, fill ? 1 : 0);
        lcd_update();
        msleep(150);
    }
}

/*============================================================================
 * Update Helper Functions
 *===========================================================================*/

static void UpdateAutoAdvance(void)
{
    if (Queue_IsEmpty()) {
        return;
    }
    
    /* Check if auto-advance interval has passed */
    if (get_tick_count() - g_system.queue.last_advance_tick >= QUEUE_AUTO_ADVANCE_MS) {
        Queue_CallNext();
        
        /* Show brief notification */
        char buffer[16];
        Queue_FormatTicket(buffer, g_system.queue.current_prefix, 
                          g_system.queue.current_number);
        Display_ShowAlert("NOW CALLING", buffer, 2000);
    }
}

static void UpdateBlinkState(void)
{
    if (get_tick_count() - g_system.display.last_blink_tick >= QUEUE_BLINK_INTERVAL_MS) {
        g_system.display.blink_state = !g_system.display.blink_state;
        g_system.display.last_blink_tick = get_tick_count();
        
        /* Refresh display for blink effect */
        if (g_system.display.current_screen == SCREEN_QUEUE_STATUS) {
            g_system.display.needs_refresh = true;
        }
    }
    
    /* Clear LED pattern after brief display */
    static uint32_t led_clear_tick = 0;
    if (g_system.led_pattern != 0) {
        if (led_clear_tick == 0) {
            led_clear_tick = get_tick_count();
        } else if (get_tick_count() - led_clear_tick >= 200) {
            g_system.led_pattern = 0;
            led_clear_tick = 0;
        }
    }
}

/*============================================================================
 * Button Handling
 *===========================================================================*/

ButtonAction_t Button_Read(void)
{
    uint32_t current_tick = get_tick_count();
    
    /* Debounce check */
    if (current_tick - g_last_button_tick < BUTTON_DEBOUNCE_MS) {
        return BTN_NONE;
    }
    
    /* Read physical buttons */
    uint8_t btn_state = TERASIC_BUTTON_Read();
    
    /* Detect rising edges (new presses) */
    uint8_t pressed = btn_state & ~g_last_button_state;
    g_last_button_state = btn_state;
    
    if (pressed == 0) {
        return BTN_NONE;
    }
    
    g_last_button_tick = current_tick;
    
    /* Map to button actions */
    if (pressed & 0x01) return BTN_NEXT;
    if (pressed & 0x02) return BTN_ADD;
    if (pressed & 0x04) return BTN_TOGGLE;
    if (pressed & 0x08) return BTN_RESET;
    
    return BTN_NONE;
}

void Button_Process(ButtonAction_t action)
{
    char buffer[32];
    
    switch (action) {
        case BTN_NEXT:
            /* Call next customer */
            if (Queue_CallNext()) {
                Queue_FormatTicket(buffer, g_system.queue.current_prefix,
                                  g_system.queue.current_number);
                Display_ShowAlert("NOW CALLING", buffer, 1500);
                g_system.led_pattern = 0x01;
            } else {
                Display_ShowAlert("QUEUE EMPTY", "No customers", 1500);
            }
            break;
            
        case BTN_ADD:
            /* Add new customer */
            if (!Queue_IsFull()) {
                uint16_t ticket = Queue_AddCustomer(PRIORITY_NORMAL);
                Queue_FormatTicket(buffer, g_system.queue.current_prefix, ticket);
                Display_ShowAlert("NEW CUSTOMER", buffer, 1500);
                g_system.led_pattern = 0x02;
            } else {
                Display_ShowAlert("QUEUE FULL", "Max capacity", 1500);
            }
            break;
            
        case BTN_TOGGLE:
            /* Toggle between queue and statistics screens */
            if (g_system.display.current_screen == SCREEN_QUEUE_STATUS) {
                Display_SetScreen(SCREEN_STATISTICS);
            } else if (g_system.display.current_screen == SCREEN_STATISTICS) {
                Display_SetScreen(SCREEN_QUEUE_STATUS);
            } else {
                Display_SetScreen(SCREEN_QUEUE_STATUS);
            }
            g_system.led_pattern = 0x04;
            break;
            
        case BTN_RESET:
            /* Reset system */
            Display_ShowAlert("SYSTEM RESET", "Clearing...", 1000);
            msleep(1000);
            QueueSystem_Reset();
            g_system.led_pattern = 0x08;
            break;
            
        default:
            break;
    }
}

void Button_Simulate(ButtonAction_t action)
{
    Button_Process(action);
}

/*============================================================================
 * Time Functions
 *===========================================================================*/

void Time_Init(bool simulated)
{
    g_system.time.hours = 0;
    g_system.time.minutes = 0;
    g_system.time.seconds = 0;
    g_system.time.day = 1;
    g_system.time.month = 1;
    g_system.time.year = 2024;
    g_system.time.last_tick = get_tick_count();
    g_system.time.is_simulated = simulated;
}

void Time_Update(void)
{
    uint32_t current_tick = get_tick_count();
    uint32_t elapsed = current_tick - g_system.time.last_tick;
    
    if (g_system.time.is_simulated) {
        /* Simulated mode: 1 real second = 1 simulated minute */
        if (elapsed >= 1000) {
            g_system.time.last_tick = current_tick;
            
            g_system.time.minutes++;
            if (g_system.time.minutes >= 60) {
                g_system.time.minutes = 0;
                g_system.time.hours++;
                if (g_system.time.hours >= 24) {
                    g_system.time.hours = 0;
                    Stats_ResetDaily();  /* New day */
                }
            }
        }
    } else {
        /* Real-time mode */
        if (elapsed >= 1000) {
            g_system.time.last_tick = current_tick;
            
            g_system.time.seconds++;
            if (g_system.time.seconds >= 60) {
                g_system.time.seconds = 0;
                g_system.time.minutes++;
                if (g_system.time.minutes >= 60) {
                    g_system.time.minutes = 0;
                    g_system.time.hours++;
                    if (g_system.time.hours >= 24) {
                        g_system.time.hours = 0;
                    }
                }
            }
        }
    }
}

SystemTime_t *Time_Get(void)
{
    return &g_system.time;
}

void Time_Format(char *buffer)
{
    if (buffer != NULL) {
        sprintf(buffer, "%02d:%02d", g_system.time.hours, g_system.time.minutes);
    }
}

void Time_FormatFull(char *buffer)
{
    if (buffer != NULL) {
        sprintf(buffer, "%02d:%02d:%02d", 
                g_system.time.hours, g_system.time.minutes, g_system.time.seconds);
    }
}

/*============================================================================
 * Hardware Interface Functions
 *===========================================================================*/

void HW_SetLEDs(uint32_t pattern)
{
    g_system.led_pattern = pattern;
    TERASIC_LED_Set(pattern);
}

void HW_SetHexDisplay(uint32_t number)
{
    TERASIC_HEX_Display(number, 6);
}

uint32_t HW_ReadSwitches(void)
{
    return TERASIC_SWITCH_Read();
}

/*============================================================================
 * Demo Mode Functions
 *===========================================================================*/

void Demo_Enable(int enable)
{
    g_system.demo_mode = (enable != 0);
    if (enable) {
        g_demo_start_tick = get_tick_count();
        g_demo_step = 0;
    }
    INFO_PRINT("Demo mode %s", enable ? "enabled" : "disabled");
}

void Demo_Update(void)
{
    if (!g_system.demo_mode) {
        return;
    }
    
    uint32_t elapsed = (get_tick_count() - g_demo_start_tick) / 1000;
    
    switch (g_demo_step) {
        case 0:
            if (elapsed >= 5) {
                Button_Simulate(BTN_NEXT);
                g_demo_step++;
            }
            break;
            
        case 1:
            if (elapsed >= 10) {
                Button_Simulate(BTN_ADD);
                g_demo_step++;
            }
            break;
            
        case 2:
            if (elapsed >= 15) {
                Button_Simulate(BTN_TOGGLE);
                g_demo_step++;
            }
            break;
            
        case 3:
            if (elapsed >= 20) {
                Button_Simulate(BTN_TOGGLE);
                g_demo_step++;
            }
            break;
            
        case 4:
            if (elapsed >= 25) {
                Button_Simulate(BTN_NEXT);
                g_demo_step++;
            }
            break;
            
        case 5:
            if (elapsed >= 28) {
                Button_Simulate(BTN_ADD);
                Button_Simulate(BTN_ADD);
                g_demo_step++;
            }
            break;
            
        default:
            if (elapsed >= 35) {
                /* Reset demo cycle */
                g_demo_start_tick = get_tick_count();
                g_demo_step = 0;
            }
            break;
    }
}

void Demo_RunSequence(void)
{
    INFO_PRINT("Running demo sequence...");
    
    /* Welcome animation */
    Display_SetScreen(SCREEN_WELCOME);
    Screen_DrawWelcome();
    lcd_update();
    msleep(2000);
    
    /* Scroll in title */
    Animation_ScrollTextIn(">> SMART QUEUE <<", 28, 10);
    msleep(500);
    
    /* Initialization screen */
    Display_SetScreen(SCREEN_INIT);
    for (int pct = 0; pct <= 100; pct += 5) {
        g_system.display.animation_frame = pct;
        Screen_DrawInit();
        lcd_update();
        msleep(50);
    }
    msleep(500);
    
    /* Transition to main screen */
    Animation_WipeDown();
    Display_SetScreen(SCREEN_QUEUE_STATUS);
    g_system.display.needs_refresh = true;
    
    INFO_PRINT("Demo sequence complete, entering main loop");
}

/*============================================================================
 * Global State Access
 *===========================================================================*/

SystemState_t *System_GetState(void)
{
    return &g_system;
}

/*============================================================================
 * Welcome Sequence (called from main)
 *===========================================================================*/

void WelcomeSequence_Run(void)
{
    INFO_PRINT("Running welcome sequence...");
    
    /* Clear display */
    lcd_clear();
    lcd_update();
    msleep(300);
    
    /* Draw logo */
    uint8_t logo_x = (128 - 16) / 2;
    DRAW_Bitmap(logo_x, 5, 16, 16, logo_bitmap);
    lcd_update();
    msleep(500);
    
    /* Animate title */
    Animation_ScrollTextIn(">> SMART QUEUE <<", 28, 8);
    msleep(200);
    
    /* Typewriter subtitle */
    Animation_TypewriterText("SYSTEM DEMO", 25, 38);
    msleep(300);
    
    /* Draw decorations */
    DRAW_Line(10, 24, 117, 24, 1);
    DRAW_Line(10, 50, 117, 50, 1);
    DRAW_StringCenter(54, "FPGA-HPS Co-Design");
    lcd_update();
    msleep(1500);
    
    /* Show initialization */
    lcd_clear();
    DRAW_StringCenter(5, "INITIALIZING...");
    DRAW_String(10, 18, "[*] LCD Module");
    lcd_update();
    msleep(300);
    
    DRAW_String(10, 28, "[*] Queue Engine");
    lcd_update();
    msleep(300);
    
    DRAW_String(10, 38, "[*] Time Service");
    lcd_update();
    msleep(300);
    
    /* Progress bar */
    DRAW_String(10, 52, "Loading:");
    Animation_ProgressBar(55, 50, 65, 10, 2000);
    msleep(300);
    
    /* Transition to main screen */
    Animation_WipeDown();
    Display_SetScreen(SCREEN_QUEUE_STATUS);
    g_system.display.needs_refresh = true;
    Display_Update();
    
    INFO_PRINT("Welcome sequence complete");
}