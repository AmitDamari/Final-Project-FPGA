# FPGA-Based Smart Queue Display System
### Hardware-Software Co-Design on Intel DE10-Standard

---

## Executive Summary

This project implements a deterministic, real-time information display system for queue management, status boards, or process guidance. It transcends a simple software application by architecting a **custom heterogeneous System-on-Chip (SoC)** inside an FPGA.

The system is built on the **Intel DE10-Standard** board, leveraging its **Cyclone V SoC** to combine:
- A **Hard Processor System (HPS)** running a Linux application
- A **custom FPGA fabric** implementing timing-critical hardware peripherals

The core innovation is a disciplined **hardware-software co-design** where a high-level C application manages logic and data flow, while dedicated Verilog hardware modules guarantee pixel-perfect timing for a graphical LCD and instantaneous response to user inputs.

---

## 1. Core Architectural Philosophy

### 1.1 The Hardware-Software Partitioning Decision

The system's architecture is a direct response to the limitations of pure software or pure hardware solutions:
- A Raspberry Pi running Python could display messages but suffers from **non-deterministic timing**, boot delays, and potential OS freezes.
- A pure hardware FSM in Verilog could be fast and reliable but would be **inflexible and complex** to modify.

#### Our Co-Design Solution

|
 Layer 
|
 Implementation 
|
 Responsibility 
|
|
-------
|
----------------
|
----------------
|
|
**
"Brain"
**
|
 C application on HPS (ARM Cortex-A9) 
|
 UI state machine, message queue management, system logic. Provides 
**
flexibility and complex decision-making
**
. 
|
|
**
"Muscles & Nerves"
**
|
 Custom Verilog in FPGA fabric 
|
 Dedicated hardware accelerators for 
**
deterministic timing, parallel execution, and high reliability
**
. Drives LCD with nanosecond-accurate pulses; debounces buttons in real-time. 
|

### 1.2 Communication Fabric: The HPS-FPGA Bridge

The HPS and FPGA do not speak the same language natively:
- HPS uses **AMBA AXI** bus
- Custom peripherals use Intel's **Avalon Memory-Mapped (Avalon-MM)** interface

**The Critical Bridge:** The **HPS-to-FPGA Lightweight AXI Bridge**, configured in Intel Platform Designer (Qsys), performs real-time protocol translation. The ARM processor reads/writes to Verilog peripherals as simple memory locations. All communication is **memory-mapped I/O**, creating a clean, addressable hardware-software interface.

---

## 2. Repository Structure
final-project-fpga/
├── de10_standard.qpf # Quartus Prime Project File
├── README.md # This document
├── .gitignore # Excludes binaries, waveforms, Quartus output
│
├── hw/ # ── Hardware Design ──
│ ├── rtl/ # Synthesizable Verilog source
│ │ ├── top_level.v # Top-level entity (instantiates Qsys system)
│ │ ├── lcd_controller.v # LCD graphics pipeline
│ │ ├── uart_transmitter.v # 9600 8N1 UART TX
│ │ ├── button_debouncer.v # Input conditioning
│ │ ├── clock_divider.v # Derived clock enables
│ │ ├── programmable_timer.v # Loadable down-counter
│ │ ├── message_memory.v # 16-message ROM
│ │ ├── main_fsm.v # Hardware state machine
│ │ └── ...
│ ├── qsys/ # Platform Designer systems
│ │ └── nios_system.qsys # [CRITICAL] HPS, bridge, memory, custom IP
│ └── constraints/ # Physical & timing constraints
│ ├── de10_standard.qsf # [CRITICAL] Pin assignments
│ └── de10_standard.sdc # [CRITICAL] Timing constraints (50MHz clock)
│
├── sim/ # ── Simulation & Verification ──
│ ├── testbenches/ # Verilog testbenches
│ │ ├── tb_uart_transmitter.v
│ │ ├── tb_lcd_controller.v
│ │ └── ...
│ ├── waves/ # .vcd waveform outputs (git-ignored)
│ └── scripts/ # TCL/shell automation scripts
│
└── sw/ # ── Software (HPS / ARM) ──
├── src/
│ ├── main.c # Primary application state machine
│ └── lcd_driver.c # Hardware abstraction layer
└── inc/
├── addresses.h # [SINGLE SOURCE OF TRUTH] Memory map from Qsys
├── lcd_driver.h # API: lcd_init(), lcd_print(), etc.
└── regs.h # Bit-field definitions for HW registers

text

---

## 3. Toolchain & Collaboration

| Role | Owner | Tools | Deliverables |
|------|-------|-------|--------------|
| **FPGA Integrator** | Amit | Quartus Prime Pro, Platform Designer | `.sof` bitstream, `addresses.h`, pin/timing constraints |
| **Module & SW Developer** | Ido | VS Code, ModelSim/Questa or iverilog+gtkwave, ARM cross-compiler | Verified RTL modules, testbenches, C application |

**Sync Protocol:**
- Commit and push daily
- `sw/inc/addresses.h` is the contract between HW and SW
- Prefer small incremental merges over long-lived branches

---

## 4. Hardware Module Deep Dive (`hw/rtl/`)

Each module is a self-contained IP core with a standardized Avalon-MM slave interface.

### 4.1 LCD Controller (`lcd_controller.v`)
> **The "Crown Jewel" – A dedicated graphics processor**

**Purpose:** Manages the entire protocol and timing for the **128×64 pixel KS0108B graphic LCD**.

**Core Challenge:** Bridging the fast FPGA domain (20ns clock cycles) with slow LCD requirements (μs-scale pulse widths).

**Sub-Blocks:**
| Block | Function |
|-------|----------|
| **Avalon-MM Slave** | Presents register file to HPS (Data Reg, Control/Status Reg) |
| **Dual-Port Frame Buffer** | 1K×8 RAM. Port A for CPU writes, Port B for rendering. Prevents tearing. |
| **Timing Generator** | FSM producing exact `LCD_EN`, `LCD_RS`, `LCD_RW`, `LCD_CS1/2` sequences per KS0108B datasheet (EN pulse > 450ns) |
| **Font ROM** | ASCII code + row index → 8-bit pixel row |

**Power-On Init Sequence:** `0x3F` (Display On) → `0xC0` (Set Start Line)

---

### 4.2 UART Transmitter (`uart_transmitter.v`)

**Purpose:** Classic serializer for **9600 baud, 8N1** protocol.

**Interface:**
| Signal | Direction | Description |
|--------|-----------|-------------|
| `tx_data[7:0]` | Input | Byte to transmit |
| `tx_start` | Input | Pulse to begin transmission |
| `tx_busy` | Output | High while transmitting |

**Operation:** Generates Start Bit (0) → 8 data bits (LSB first) → Stop Bit (1). The `lcd_controller` polls `tx_busy` between characters.

---

### 4.3 Button Debouncer (`button_debouncer.v`)

**Purpose:** Converts noisy mechanical button signals into clean, single-cycle digital pulses.

**Three-Stage Pipeline:**
1. **Synchronizer (2 FFs):** Guards against metastability at 50MHz domain entry
2. **Debounce Counter:** Requires ~20ms stability (~1M cycles) before state change
3. **Edge Detector:** Produces `pulse_button_next` / `pulse_button_back` on clean rising edge

---

### 4.4 Timing Subsystem

| Module | Function |
|--------|----------|
| `clock_divider.v` | Derives enable ticks from 50MHz (e.g., `tick_1ms`, `tick_1s`) |
| `programmable_timer.v` | Loadable down-counter. HPS writes value; asserts `timeout_irq` at zero. Readable count register for debug display. |

---

### 4.5 Message Storage (`message_memory.v`)

**Purpose:** Pre-initialized ROM storing 16 display messages.

**Organization:** 16 entries × 128 bits (16 chars × 8 bits/char)

**Access:** HPS reads each message via **four sequential 32-bit Avalon-MM transactions**.

---

### 4.6 Top Level (`top_level.v`)

**Purpose:** Instantiates entire Qsys system and maps conduit interfaces to physical DE10-Standard pins.

Example mappings:
- `GPIO_0[0]` → `uart_tx`
- `KEY[0]` → `reset_n`

---

## 5. Software Architecture (`sw/`)

### 5.1 Hardware Abstraction Layer (`lcd_driver.c/.h`)

**Purpose:** Provides clean, high-level API hiding memory-mapped I/O complexity.

**Implementation:**
- Uses Linux `mmap()` to map FPGA peripheral addresses (from `addresses.h`) into virtual memory
- `lcd_print()` writes to `lcd_controller` data register, polling `tx_busy` between characters

**Example API:**
```c
void lcd_init(void);
void lcd_clear(void);
void lcd_print(const char* str);
void lcd_set_cursor(uint8_t row, uint8_t col);
5.2 Main Application (main.c)
Purpose: Implements system behavior—cycling messages, button response, auto-advance timers.

State Machine:

State	Description
BOOT	Wait for hardware initialization
IDLE	Display "READY", wait for button_next
DISPLAY_ACTIVE	Show current message from ROM, start timer
WAIT_TIMEOUT	Poll timer status; on timeout or button press, advance message
6. Verification Strategy
We employ multi-stage verification to ensure correctness before hardware deployment.

Stage 1: Unit Testing (VS Code)
Each module has dedicated testbench (tb_*.v)
Verify functionality in isolation
Success metric: Correct .vcd waveforms
Stage 2: Integration Testing (Quartus)
ModelSim with NativeLink simulates entire Qsys system
Verify Avalon-MM bus traffic between HPS model and peripherals
Stage 3: Hardware-in-the-Loop Debugging
Feature	Purpose
Timer on LCD	Software displays timer count for visual timing confirmation
FSM State on LEDs	main_fsm state → LEDR[3:0] for instant visual feedback
JTAG UART	printf()-style debug messages to host PC console
7. Development Workflow
text
┌─────────────────────────────────────────────────────────────────┐
│  1. RTL Development (Ido)                                       │
│     └── Develop & simulate module in hw/rtl/ → Push to Git      │
│                              ↓                                  │
│  2. System Integration (Amit)                                   │
│     └── Pull module → Add to Qsys → Connect Avalon → Regen      │
│                              ↓                                  │
│  3. Memory Map Update (Amit)                                    │
│     └── Export addresses → Update sw/inc/addresses.h            │
│                              ↓                                  │
│  4. Software Adaptation (Ido)                                   │
│     └── Update lcd_driver.c / main.c with new addresses         │
│                              ↓                                  │
│  5. System Test (Both)                                          │
│     └── Amit programs board → Ido runs C app → Joint debug      │
└─────────────────────────────────────────────────────────────────┘
8. Conclusion
This project is more than a message display—it is a case study in modern embedded systems design. It demonstrates:

✅ Problem partitioning into software and hardware domains
✅ Clean interface design (Avalon-MM)
✅ Reliable RTL peripheral implementation
✅ Verification-driven development
✅ Full system integration on a single chip
The final system is a robust, real-time appliance that boots instantly, responds reliably, and performs its dedicated function with the precision only custom hardware can provide.

Quick Start
bash
# Clone the repository
git clone https://github.com/AmitDamari/Final-Project-FPGA.git
cd Final-Project-FPGA/final-project-fpga

# Open in Quartus Prime Pro
# File → Open Project → de10_standard.qpf

# Compile & program the FPGA
# Processing → Start Compilation
# Tools → Programmer → Start