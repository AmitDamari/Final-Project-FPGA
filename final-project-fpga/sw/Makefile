#******************************************************************************
# Makefile for Smart Queue Display System
# For Intel DE10-Standard (Cyclone V SoC) with ARM HPS
#******************************************************************************

#==============================================================================
# Cross-Compiler Configuration
#==============================================================================

CROSS_COMPILE ?= arm-linux-gnueabihf-
CC = $(CROSS_COMPILE)gcc
STRIP = $(CROSS_COMPILE)strip

#==============================================================================
# Target Configuration
#==============================================================================

TARGET = queue_display
TARGET_DIR = /home/root
TARGET_IP ?= 192.168.1.100
TARGET_USER ?= root

#==============================================================================
# Directory Structure
#==============================================================================

SRC_DIR = src
INC_DIR = inc
OBJ_DIR = obj
BIN_DIR = bin

#==============================================================================
# Source Files (ALL source files must be listed here)
#==============================================================================

SRCS = \
    $(SRC_DIR)/main.c \
    $(SRC_DIR)/queue_system.c \
    $(SRC_DIR)/lcd_driver.c \
    $(SRC_DIR)/LCD_Driver.c \
    $(SRC_DIR)/LCD_Hw.c \
    $(SRC_DIR)/LCD_Lib.c \
    $(SRC_DIR)/lcd_graphic.c \
    $(SRC_DIR)/font.c \
    $(SRC_DIR)/terasic_lib.c

#==============================================================================
# Object Files
#==============================================================================

OBJS = $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRCS))

#==============================================================================
# Compiler Flags
#==============================================================================

INCLUDES = -I$(INC_DIR)
WARNINGS = -Wall -Wextra -Wno-unused-parameter -Wno-unused-variable
OPT_FLAGS = -O2
DEBUG_FLAGS = -g -DDEBUG
ARCH_FLAGS = -march=armv7-a -mtune=cortex-a9 -mfpu=neon -mfloat-abi=hard

CFLAGS = $(ARCH_FLAGS) $(OPT_FLAGS) $(WARNINGS) $(INCLUDES)
CFLAGS += -DSMART_QUEUE_SYSTEM -D_GNU_SOURCE

#==============================================================================
# Linker Flags
#==============================================================================

LDFLAGS = -lm -lpthread -lrt

#==============================================================================
# Build Rules
#==============================================================================

.PHONY: all clean install run debug directories help

all: directories $(BIN_DIR)/$(TARGET)
	@echo "========================================"
	@echo "  Build complete: $(BIN_DIR)/$(TARGET)"
	@echo "========================================"

directories:
	@mkdir -p $(OBJ_DIR)
	@mkdir -p $(BIN_DIR)

$(BIN_DIR)/$(TARGET): $(OBJS)
	@echo "[LD] Linking $@..."
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@echo "[CC] Compiling $<..."
	$(CC) $(CFLAGS) -c $< -o $@

debug: CFLAGS += $(DEBUG_FLAGS)
debug: OPT_FLAGS = -O0
debug: clean all

release: all
	$(STRIP) $(BIN_DIR)/$(TARGET)

#==============================================================================
# Deployment
#==============================================================================

install: all
	@echo "[INSTALL] Copying to $(TARGET_USER)@$(TARGET_IP):$(TARGET_DIR)/"
	scp $(BIN_DIR)/$(TARGET) $(TARGET_USER)@$(TARGET_IP):$(TARGET_DIR)/
	ssh $(TARGET_USER)@$(TARGET_IP) "chmod +x $(TARGET_DIR)/$(TARGET)"

run: install
	ssh -t $(TARGET_USER)@$(TARGET_IP) "cd $(TARGET_DIR) && ./$(TARGET)"

run-demo: install
	ssh -t $(TARGET_USER)@$(TARGET_IP) "cd $(TARGET_DIR) && ./$(TARGET) --demo"

#==============================================================================
# Clean
#==============================================================================

clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)

#==============================================================================
# Help
#==============================================================================

help:
	@echo "Targets:"
	@echo "  all       - Build the application"
	@echo "  clean     - Remove build artifacts"
	@echo "  debug     - Build with debug symbols"
	@echo "  install   - Copy to target board"
	@echo "  run       - Run on target board"
	@echo "  run-demo  - Run in demo mode"