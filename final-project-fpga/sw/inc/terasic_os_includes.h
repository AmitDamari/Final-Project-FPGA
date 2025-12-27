/******************************************************************************
 * terasic_os_includes.h
 * 
 * OS-specific includes and definitions for Terasic DE10-Standard
 * Linux HPS environment
 *
 ******************************************************************************/

#ifndef TERASIC_OS_INCLUDES_H_
#define TERASIC_OS_INCLUDES_H_

/*============================================================================
 * Standard Library Includes
 *===========================================================================*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>
#include <signal.h>
#include <pthread.h>

/*============================================================================
 * Linux-specific Includes
 *===========================================================================*/

#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/time.h>

/*============================================================================
 * Type Definitions
 *===========================================================================*/

typedef uint8_t     alt_u8;
typedef uint16_t    alt_u16;
typedef uint32_t    alt_u32;
typedef uint64_t    alt_u64;

typedef int8_t      alt_8;
typedef int16_t     alt_16;
typedef int32_t     alt_32;
typedef int64_t     alt_64;

typedef volatile uint32_t   vuint32_t;
typedef volatile uint16_t   vuint16_t;
typedef volatile uint8_t    vuint8_t;

/*============================================================================
 * Boolean Definitions
 *===========================================================================*/

#ifndef TRUE
#define TRUE    1
#endif

#ifndef FALSE
#define FALSE   0
#endif

#ifndef NULL
#define NULL    ((void *)0)
#endif

/*============================================================================
 * HPS Memory Map Constants
 *===========================================================================*/

/* HPS-to-FPGA Bridge Base Addresses */
#define HPS_FPGA_BRIDGE_BASE        0xC0000000  /* H2F Bridge */
#define HPS_LW_BRIDGE_BASE          0xFF200000  /* Lightweight H2F Bridge */
#define HPS_FPGA_BRIDGE_SPAN        0x04000000  /* 64 MB */
#define HPS_LW_BRIDGE_SPAN          0x00200000  /* 2 MB */

/* GPIO Base Addresses */
#define HPS_GPIO0_BASE              0xFF708000
#define HPS_GPIO1_BASE              0xFF709000
#define HPS_GPIO2_BASE              0xFF70A000

/* System Manager */
#define SYSMGR_BASE                 0xFFD08000

/*============================================================================
 * Utility Macros
 *===========================================================================*/

#define ARRAY_SIZE(x)               (sizeof(x) / sizeof((x)[0]))
#define MIN(a, b)                   (((a) < (b)) ? (a) : (b))
#define MAX(a, b)                   (((a) > (b)) ? (a) : (b))
#define CLAMP(x, low, high)         (((x) > (high)) ? (high) : (((x) < (low)) ? (low) : (x)))

/* Memory-mapped I/O helpers */
#define IORD(base, offset)          (*((volatile uint32_t *)((base) + (offset))))
#define IOWR(base, offset, data)    (*((volatile uint32_t *)((base) + (offset))) = (data))

#define IORD_8DIRECT(base, offset)  (*((volatile uint8_t *)((base) + (offset))))
#define IOWR_8DIRECT(base, offset, data) (*((volatile uint8_t *)((base) + (offset))) = (data))

#define IORD_16DIRECT(base, offset) (*((volatile uint16_t *)((base) + (offset))))
#define IOWR_16DIRECT(base, offset, data) (*((volatile uint16_t *)((base) + (offset))) = (data))

#define IORD_32DIRECT(base, offset) (*((volatile uint32_t *)((base) + (offset))))
#define IOWR_32DIRECT(base, offset, data) (*((volatile uint32_t *)((base) + (offset))) = (data))

/*============================================================================
 * Debug Macros
 *===========================================================================*/

#ifdef DEBUG
    #define DEBUG_PRINT(fmt, ...) \
        fprintf(stderr, "[DEBUG] %s:%d: " fmt "\n", __func__, __LINE__, ##__VA_ARGS__)
#else
    #define DEBUG_PRINT(fmt, ...)
#endif

#define INFO_PRINT(fmt, ...) \
    printf("[INFO] " fmt "\n", ##__VA_ARGS__)

#define ERROR_PRINT(fmt, ...) \
    fprintf(stderr, "[ERROR] %s:%d: " fmt "\n", __func__, __LINE__, ##__VA_ARGS__)

#define WARNING_PRINT(fmt, ...) \
    fprintf(stderr, "[WARNING] " fmt "\n", ##__VA_ARGS__)

#endif /* TERASIC_OS_INCLUDES_H_ */