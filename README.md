# FPGA Smart Queue System

## Architecture
- **Platform**: Intel MAX 10 (DE10-standrard)
- **Soft-Core**: Nios II (Controls complex Logic)
- **RTL modules**: Handle fast timing (UART, Timers, Debouncing)

## Hardware Structure (hw/rtl)
- **baud_generator.v**: 9600 bps tick
- **uart_transmitter.v**: 8N1 Serial Output
- **main_fsm.v**: Hardware Logic
- **clock_divider.v**: System heartbeat

## Simulation
Scripts are in sim/scripts/.
