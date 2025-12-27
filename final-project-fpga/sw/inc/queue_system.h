/******************************************************************************
 * queue_system.h
 * 
 * Smart Queue System - Core queue management definitions
 * For FPGA-Based Smart Queue Display System
 *
 ******************************************************************************/

#ifndef QUEUE_SYSTEM_H_
#define QUEUE_SYSTEM_H_

#include "terasic_os_includes.h"

/*============================================================================
 * Queue System Configuration
 *===========================================================================*/

#define QUEUE_MAX_SIZE              50      /* Maximum customers in queue */
#define QUEUE_PREFIX_DEFAULT        'A'     /* Default queue prefix */
#define QUEUE_START_NUMBER          100     /* Starting ticket number */
#define QUEUE_AUTO_ADVANCE_MS       30000   /* Auto-advance interval (30s) */
#define QUEUE_BLINK_INTERVAL_MS     500     /* Blink interval for alerts */

/* Feature Enables */
#define FEATURE_DEMO_MODE           1       /* Enable demo/simulation mode */
#define FEATURE_CONSOLE_DEBUG       1       /* Enable console debug output */
#define FEATURE_SOUND_ALERTS        0       /* Enable sound notifications */
#define FEATURE_NETWORK             0       /* Enable network connectivity */

/*============================================================================
 * Screen States
 *===========================================================================*/

typedef enum {
    SCREEN_BOOT,                /* Boot/splash screen */
    SCREEN_WELCOME,             /* Welcome animation */
    SCREEN_INIT,                /* Initialization progress */
    SCREEN_QUEUE_STATUS,        /* Main queue display */
    SCREEN_STATISTICS,          /* Statistics display */
    SCREEN_SETTINGS,            /* Settings menu */
    SCREEN_ALERT,               /* Alert/notification overlay */
    SCREEN_TRANSITION           /* Screen transition */
} ScreenState_t;

/*============================================================================
 * Button Actions
 *===========================================================================*/

typedef enum {
    BTN_NONE        = 0x00,
    BTN_NEXT        = 0x01,     /* KEY0: Call next customer */
    BTN_ADD         = 0x02,     /* KEY1: Add new customer */
    BTN_TOGGLE      = 0x04,     /* KEY2: Toggle screen */
    BTN_RESET       = 0x08      /* KEY3: Reset system */
} ButtonAction_t;

/*============================================================================
 * Customer Priority Levels
 *===========================================================================*/

typedef enum {
    PRIORITY_NORMAL = 0,
    PRIORITY_ELDERLY,
    PRIORITY_DISABLED,
    PRIORITY_VIP
} CustomerPriority_t;

/*============================================================================
 * Customer Structure
 *===========================================================================*/

typedef struct {
    char prefix;                /* Queue prefix (A, B, C...) */
    uint16_t number;            /* Ticket number */
    uint32_t arrive_time;       /* Arrival timestamp (ms) */
    uint32_t serve_time;        /* Service start timestamp (ms) */
    CustomerPriority_t priority;/* Priority level */
    uint8_t service_type;       /* Type of service requested */
    bool served;                /* Has been served flag */
    bool no_show;               /* Customer didn't show up */
} Customer_t;

/*============================================================================
 * Queue Statistics
 *===========================================================================*/

typedef struct {
    /* Daily counters */
    uint16_t total_today;       /* Total tickets issued today */
    uint16_t served_today;      /* Customers served today */
    uint16_t no_shows_today;    /* No-shows today */
    uint16_t waiting_count;     /* Currently waiting */
    
    /* Timing statistics */
    float avg_wait_time;        /* Average wait time (minutes) */
    float avg_service_time;     /* Average service time (minutes) */
    uint32_t total_wait_time;   /* Sum of all wait times (ms) */
    uint32_t total_service_time;/* Sum of all service times (ms) */
    
    /* Hourly breakdown (24 hours) */
    uint16_t hourly_customers[24];
    uint16_t hourly_served[24];
    
    /* Peak tracking */
    uint8_t peak_hour;          /* Hour with most customers */
    uint16_t peak_count;        /* Peak customer count */
    
    /* Performance metrics */
    uint16_t longest_wait;      /* Longest wait time (minutes) */
    uint16_t shortest_wait;     /* Shortest wait time (minutes) */
} QueueStats_t;

/*============================================================================
 * System Time (Simulated/Real)
 *===========================================================================*/

typedef struct {
    uint8_t hours;
    uint8_t minutes;
    uint8_t seconds;
    uint8_t day;
    uint8_t month;
    uint16_t year;
    uint32_t last_tick;         /* Last update tick count */
    bool is_simulated;          /* Using simulated time */
} SystemTime_t;

/*============================================================================
 * Display State
 *===========================================================================*/

typedef struct {
    ScreenState_t current_screen;
    ScreenState_t previous_screen;
    ScreenState_t next_screen;
    bool needs_refresh;
    bool is_animating;
    uint8_t animation_frame;
    uint32_t animation_start;
    bool blink_state;
    uint32_t last_blink_tick;
    uint8_t brightness;
    bool backlight_on;
} DisplayState_t;

/*============================================================================
 * Queue System State
 *===========================================================================*/

typedef struct {
    /* Queue data */
    Customer_t customers[QUEUE_MAX_SIZE];
    uint8_t head;               /* Index of next to serve */
    uint8_t tail;               /* Index for new customers */
    uint8_t count;              /* Number waiting */
    
    /* Current service */
    char current_prefix;
    uint16_t current_number;    /* Currently serving */
    uint16_t next_ticket;       /* Next ticket to issue */
    
    /* Timing */
    uint32_t last_advance_tick;
    uint32_t service_start_tick;
    
    /* State */
    bool is_active;
    bool is_paused;
    bool auto_advance_enabled;
} QueueState_t;

/*============================================================================
 * Main System State
 *===========================================================================*/

typedef struct {
    QueueState_t queue;
    QueueStats_t stats;
    DisplayState_t display;
    SystemTime_t time;
    
    /* Hardware state */
    uint32_t led_pattern;
    uint8_t hex_display[6];
    
    /* Flags */
    bool initialized;
    bool running;
    bool demo_mode;
    
    /* FPGA communication */
    void *fpga_lw_base;
    void *fpga_h2f_base;
    int mem_fd;
} SystemState_t;

/*============================================================================
 * Function Prototypes - System Control
 *===========================================================================*/

/**
 * @brief Initialize the queue system
 * @return 0 on success, -1 on failure
 */
int QueueSystem_Init(void);

/**
 * @brief Deinitialize the queue system
 */
void QueueSystem_DeInit(void);

/**
 * @brief Main system update (call periodically)
 */
void QueueSystem_Update(void);

/**
 * @brief Reset the queue system
 */
void QueueSystem_Reset(void);

/**
 * @brief Start the queue system
 */
void QueueSystem_Start(void);

/**
 * @brief Stop the queue system
 */
void QueueSystem_Stop(void);

/**
 * @brief Pause/unpause the queue
 * @param pause 1=pause, 0=unpause
 */
void QueueSystem_Pause(int pause);

/*============================================================================
 * Function Prototypes - Queue Operations
 *===========================================================================*/

/**
 * @brief Add a new customer to the queue
 * @param priority Customer priority level
 * @return Ticket number, or 0 on failure
 */
uint16_t Queue_AddCustomer(CustomerPriority_t priority);

/**
 * @brief Call the next customer
 * @return true if successful, false if queue empty
 */
bool Queue_CallNext(void);

/**
 * @brief Mark current customer as served
 */
void Queue_MarkServed(void);

/**
 * @brief Mark current customer as no-show
 */
void Queue_MarkNoShow(void);

/**
 * @brief Get number of waiting customers
 * @return Number of customers in queue
 */
uint8_t Queue_GetWaitingCount(void);

/**
 * @brief Get customer at queue position
 * @param position Position in queue (0 = next)
 * @return Pointer to customer, or NULL
 */
Customer_t *Queue_GetCustomerAt(uint8_t position);

/**
 * @brief Get currently serving number
 * @param prefix Output: prefix character
 * @param number Output: ticket number
 */
void Queue_GetCurrentServing(char *prefix, uint16_t *number);

/**
 * @brief Format ticket number as string
 * @param buffer Output buffer
 * @param prefix Prefix character
 * @param number Ticket number
 */
void Queue_FormatTicket(char *buffer, char prefix, uint16_t number);

/**
 * @brief Check if queue is empty
 * @return true if empty
 */
bool Queue_IsEmpty(void);

/**
 * @brief Check if queue is full
 * @return true if full
 */
bool Queue_IsFull(void);

/*============================================================================
 * Function Prototypes - Statistics
 *===========================================================================*/

/**
 * @brief Get queue statistics
 * @return Pointer to statistics structure
 */
QueueStats_t *Stats_Get(void);

/**
 * @brief Reset daily statistics
 */
void Stats_ResetDaily(void);

/**
 * @brief Update statistics after service
 * @param wait_time Wait time in milliseconds
 * @param service_time Service time in milliseconds
 */
void Stats_UpdateService(uint32_t wait_time, uint32_t service_time);

/**
 * @brief Get average wait time
 * @return Average wait in minutes
 */
float Stats_GetAvgWaitTime(void);

/*============================================================================
 * Function Prototypes - Display
 *===========================================================================*/

/**
 * @brief Set current screen
 * @param screen Screen to display
 */
void Display_SetScreen(ScreenState_t screen);

/**
 * @brief Get current screen
 * @return Current screen state
 */
ScreenState_t Display_GetScreen(void);

/**
 * @brief Force display refresh
 */
void Display_Refresh(void);

/**
 * @brief Update display (call periodically)
 */
void Display_Update(void);

/**
 * @brief Show alert message
 * @param title Alert title
 * @param message Alert message
 * @param duration_ms Display duration (0 = until dismissed)
 */
void Display_ShowAlert(const char *title, const char *message, uint32_t duration_ms);

/**
 * @brief Dismiss current alert
 */
void Display_DismissAlert(void);

/*============================================================================
 * Function Prototypes - Button Handling
 *===========================================================================*/

/**
 * @brief Read button states (with debouncing)
 * @return Button action flags
 */
ButtonAction_t Button_Read(void);

/**
 * @brief Process button action
 * @param action Button action to process
 */
void Button_Process(ButtonAction_t action);

/**
 * @brief Simulate button press (for demo mode)
 * @param action Button action to simulate
 */
void Button_Simulate(ButtonAction_t action);

/*============================================================================
 * Function Prototypes - Time
 *===========================================================================*/

/**
 * @brief Initialize time system
 * @param simulated Use simulated time (accelerated)
 */
void Time_Init(bool simulated);

/**
 * @brief Update time (call periodically)
 */
void Time_Update(void);

/**
 * @brief Get current time
 * @return Pointer to time structure
 */
SystemTime_t *Time_Get(void);

/**
 * @brief Format time as string (HH:MM)
 * @param buffer Output buffer
 */
void Time_Format(char *buffer);

/**
 * @brief Format time as string with seconds (HH:MM:SS)
 * @param buffer Output buffer
 */
void Time_FormatFull(char *buffer);

/*============================================================================
 * Function Prototypes - Hardware Interface
 *===========================================================================*/

/**
 * @brief Set LED pattern
 * @param pattern LED bit pattern
 */
void HW_SetLEDs(uint32_t pattern);

/**
 * @brief Set 7-segment display
 * @param number Number to display
 */
void HW_SetHexDisplay(uint32_t number);

/**
 * @brief Read switch states
 * @return Switch bit pattern
 */
uint32_t HW_ReadSwitches(void);

/*============================================================================
 * Function Prototypes - Demo Mode
 *===========================================================================*/

/**
 * @brief Enable/disable demo mode
 * @param enable 1=enable, 0=disable
 */
void Demo_Enable(int enable);

/**
 * @brief Update demo (call periodically)
 */
void Demo_Update(void);

/**
 * @brief Run demo sequence
 */
void Demo_RunSequence(void);

/*============================================================================
 * Global System State Access
 *===========================================================================*/

/**
 * @brief Get pointer to system state
 * @return Pointer to global system state
 */
SystemState_t *System_GetState(void);

#endif /* QUEUE_SYSTEM_H_ */