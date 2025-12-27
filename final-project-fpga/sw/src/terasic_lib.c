/******************************************************************************
 * terasic_lib.c
 * 
 * Terasic Library - Common utility functions for DE10-Standard
 *
 ******************************************************************************/

#include "terasic_lib.h"
#include "addresses.h"

/*============================================================================
 * Private Variables
 *===========================================================================*/

static int g_mem_fd = -1;
static void *g_lw_bridge_base = NULL;
static void *g_h2f_bridge_base = NULL;
static bool g_initialized = false;

/*============================================================================
 * Memory Mapping Functions
 *===========================================================================*/

void *TERASIC_MEM_Open(void)
{
    if (g_initialized) {
        return g_lw_bridge_base;
    }
    
    /* Open /dev/mem */
    g_mem_fd = open("/dev/mem", O_RDWR | O_SYNC);
    if (g_mem_fd < 0) {
        ERROR_PRINT("Failed to open /dev/mem: %s", strerror(errno));
        return NULL;
    }
    
    /* Map Lightweight HPS-to-FPGA bridge */
    g_lw_bridge_base = mmap(NULL, FPGA_LW_BRIDGE_SPAN,
                            PROT_READ | PROT_WRITE, MAP_SHARED,
                            g_mem_fd, FPGA_LW_BRIDGE_BASE);
    
    if (g_lw_bridge_base == MAP_FAILED) {
        ERROR_PRINT("Failed to mmap LW bridge: %s", strerror(errno));
        close(g_mem_fd);
        g_mem_fd = -1;
        return NULL;
    }
    
    /* Map full HPS-to-FPGA bridge (optional) */
    g_h2f_bridge_base = mmap(NULL, FPGA_H2F_BRIDGE_SPAN,
                             PROT_READ | PROT_WRITE, MAP_SHARED,
                             g_mem_fd, FPGA_H2F_BRIDGE_BASE);
    
    if (g_h2f_bridge_base == MAP_FAILED) {
        WARNING_PRINT("Failed to mmap H2F bridge (non-critical)");
        g_h2f_bridge_base = NULL;
    }
    
    g_initialized = true;
    INFO_PRINT("Memory mapping initialized");
    INFO_PRINT("  LW Bridge: %p", g_lw_bridge_base);
    INFO_PRINT("  H2F Bridge: %p", g_h2f_bridge_base);
    
    return g_lw_bridge_base;
}

void TERASIC_MEM_Close(void *virtual_base)
{
    (void)virtual_base;  /* Unused parameter */
    
    if (!g_initialized) {
        return;
    }
    
    if (g_h2f_bridge_base != NULL && g_h2f_bridge_base != MAP_FAILED) {
        munmap(g_h2f_bridge_base, FPGA_H2F_BRIDGE_SPAN);
        g_h2f_bridge_base = NULL;
    }
    
    if (g_lw_bridge_base != NULL && g_lw_bridge_base != MAP_FAILED) {
        munmap(g_lw_bridge_base, FPGA_LW_BRIDGE_SPAN);
        g_lw_bridge_base = NULL;
    }
    
    if (g_mem_fd >= 0) {
        close(g_mem_fd);
        g_mem_fd = -1;
    }
    
    g_initialized = false;
    INFO_PRINT("Memory mapping closed");
}

void *TERASIC_GetLWBridgeBase(void)
{
    if (!g_initialized) {
        TERASIC_MEM_Open();
    }
    return g_lw_bridge_base;
}

void *TERASIC_GetH2FBridgeBase(void)
{
    if (!g_initialized) {
        TERASIC_MEM_Open();
    }
    return g_h2f_bridge_base;
}

/*============================================================================
 * Time Functions
 *===========================================================================*/

uint32_t get_tick_count(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint32_t)(ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
}

void msleep(uint32_t ms)
{
    usleep(ms * 1000);
}

void usleep_range(uint32_t min_us, uint32_t max_us)
{
    /* Use average of min and max */
    uint32_t sleep_us = (min_us + max_us) / 2;
    usleep(sleep_us);
}

uint64_t get_timestamp_us(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000ULL + ts.tv_nsec / 1000;
}

/*============================================================================
 * GPIO Functions
 *===========================================================================*/

int TERASIC_GPIO_Init(void)
{
    if (!g_initialized) {
        if (TERASIC_MEM_Open() == NULL) {
            return -1;
        }
    }
    return 0;
}

void TERASIC_LED_Set(uint32_t led_pattern)
{
    if (g_lw_bridge_base == NULL) return;
    
    volatile uint32_t *led_reg = (volatile uint32_t *)
        ((uint8_t *)g_lw_bridge_base + LED_BASE + LED_DATA_REG_OFFSET);
    *led_reg = led_pattern & 0x3FF;  /* 10 LEDs on DE10-Standard */
}

uint32_t TERASIC_LED_Get(void)
{
    if (g_lw_bridge_base == NULL) return 0;
    
    volatile uint32_t *led_reg = (volatile uint32_t *)
        ((uint8_t *)g_lw_bridge_base + LED_BASE + LED_DATA_REG_OFFSET);
    return *led_reg & 0x3FF;
}

uint32_t TERASIC_BUTTON_Read(void)
{
    if (g_lw_bridge_base == NULL) return 0;
    
    volatile uint32_t *btn_reg = (volatile uint32_t *)
        ((uint8_t *)g_lw_bridge_base + BUTTON_BASE + BUTTON_DATA_REG_OFFSET);
    
    /* Buttons are active low on DE10-Standard, invert for positive logic */
    return (~(*btn_reg)) & 0x0F;
}

uint32_t TERASIC_SWITCH_Read(void)
{
    if (g_lw_bridge_base == NULL) return 0;
    
    volatile uint32_t *sw_reg = (volatile uint32_t *)
        ((uint8_t *)g_lw_bridge_base + SWITCH_BASE + SWITCH_DATA_REG_OFFSET);
    return *sw_reg & 0x3FF;  /* 10 switches */
}

/*============================================================================
 * 7-Segment Display Functions
 *===========================================================================*/

/* Segment encoding: gfedcba (active low) */
static const uint8_t hex_lut[16] = {
    0x40, 0x79, 0x24, 0x30, 0x19, 0x12, 0x02, 0x78,
    0x00, 0x10, 0x08, 0x03, 0x46, 0x21, 0x06, 0x0E
};

void TERASIC_HEX_Set(int display_num, uint8_t value)
{
    if (g_lw_bridge_base == NULL) return;
    if (display_num < 0 || display_num > 5) return;
    
    uint8_t segments = (value <= 0x0F) ? hex_lut[value] : 0x7F;
    
    volatile uint32_t *hex_reg;
    
    if (display_num < 4) {
        hex_reg = (volatile uint32_t *)
            ((uint8_t *)g_lw_bridge_base + HEX_BASE + HEX0_3_REG_OFFSET);
        uint32_t current = *hex_reg;
        current &= ~(0xFF << (display_num * 8));
        current |= (segments << (display_num * 8));
        *hex_reg = current;
    } else {
        hex_reg = (volatile uint32_t *)
            ((uint8_t *)g_lw_bridge_base + HEX_BASE + HEX4_5_REG_OFFSET);
        uint32_t current = *hex_reg;
        current &= ~(0xFF << ((display_num - 4) * 8));
        current |= (segments << ((display_num - 4) * 8));
        *hex_reg = current;
    }
}

void TERASIC_HEX_Display(uint32_t number, int num_digits)
{
    if (num_digits > 6) num_digits = 6;
    
    for (int i = 0; i < num_digits; i++) {
        TERASIC_HEX_Set(i, number % 10);
        number /= 10;
    }
    
    /* Blank unused displays */
    for (int i = num_digits; i < 6; i++) {
        TERASIC_HEX_Set(i, 0xFF);  /* Blank */
    }
}

void TERASIC_HEX_Clear(void)
{
    if (g_lw_bridge_base == NULL) return;
    
    volatile uint32_t *hex03_reg = (volatile uint32_t *)
        ((uint8_t *)g_lw_bridge_base + HEX_BASE + HEX0_3_REG_OFFSET);
    volatile uint32_t *hex45_reg = (volatile uint32_t *)
        ((uint8_t *)g_lw_bridge_base + HEX_BASE + HEX4_5_REG_OFFSET);
    
    *hex03_reg = 0x7F7F7F7F;  /* All segments off */
    *hex45_reg = 0x00007F7F;
}

/*============================================================================
 * Utility Functions
 *===========================================================================*/

void TERASIC_HexDump(const void *data, size_t len)
{
    const uint8_t *p = (const uint8_t *)data;
    
    for (size_t i = 0; i < len; i += 16) {
        printf("%04zx: ", i);
        
        /* Hex bytes */
        for (size_t j = 0; j < 16; j++) {
            if (i + j < len) {
                printf("%02x ", p[i + j]);
            } else {
                printf("   ");
            }
            if (j == 7) printf(" ");
        }
        
        printf(" |");
        
        /* ASCII */
        for (size_t j = 0; j < 16 && i + j < len; j++) {
            char c = p[i + j];
            printf("%c", (c >= 32 && c < 127) ? c : '.');
        }
        
        printf("|\n");
    }
}

void TERASIC_DebugPrint(const char *format, ...)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    
    printf("[%ld.%03ld] ", ts.tv_sec % 1000, ts.tv_nsec / 1000000);
    
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
    
    printf("\n");
}