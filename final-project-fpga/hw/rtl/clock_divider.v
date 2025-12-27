// =============================================================================
// Module: clock_divider
// Description: Generates periodic enable ticks from 50MHz system clock
// Ticks: 1ms, 10ms, 100ms, 1s
// =============================================================================

module clock_divider (
    // Clock and Reset
    input  wire clk_50m,     // 50MHz system clock
    input  wire reset_n,     // Active-low reset
    
    // Output Ticks (1 clock cycle pulses)
    output wire tick_1ms,    // 1ms tick (50,000 cycles @ 50MHz)
    output wire tick_10ms,   // 10ms tick (500,000 cycles @ 50MHz)
    output wire tick_100ms,  // 100ms tick (5,000,000 cycles @ 50MHz)
    output wire tick_1s      // 1s tick (50,000,000 cycles @ 50MHz)
);

    // =========================================================================
    // Parameters
    // =========================================================================
    // Count values for each tick (0 to N-1, tick at rollover)
    localparam COUNT_1MS   = 50_000 - 1;      // 1ms / 20ns = 50,000 cycles
    localparam COUNT_10MS  = 500_000 - 1;     // 10ms / 20ns = 500,000 cycles
    localparam COUNT_100MS = 5_000_000 - 1;   // 100ms / 20ns = 5,000,000 cycles
    localparam COUNT_1S    = 50_000_000 - 1;  // 1s / 20ns = 50,000,000 cycles
    
    // Counter widths (ceil(log2(N)))
    localparam WIDTH_1MS   = $clog2(COUNT_1MS + 1);
    localparam WIDTH_10MS  = $clog2(COUNT_10MS + 1);
    localparam WIDTH_100MS = $clog2(COUNT_100MS + 1);
    localparam WIDTH_1S    = $clog2(COUNT_1S + 1);
    
    // =========================================================================
    // Internal Registers
    // =========================================================================
    reg [WIDTH_1MS-1:0]   counter_1ms;
    reg [WIDTH_10MS-1:0]  counter_10ms;
    reg [WIDTH_100MS-1:0] counter_100ms;
    reg [WIDTH_1S-1:0]    counter_1s;
    
    reg tick_1ms_reg;
    reg tick_10ms_reg;
    reg tick_100ms_reg;
    reg tick_1s_reg;
    
    // =========================================================================
    // 1ms Counter (base counter for others)
    // =========================================================================
    always @(posedge clk_50m or negedge reset_n) begin
        if (!reset_n) begin
            counter_1ms <= 0;
            tick_1ms_reg <= 1'b0;
        end else begin
            if (counter_1ms == COUNT_1MS) begin
                counter_1ms <= 0;
                tick_1ms_reg <= 1'b1;  // Generate 1-cycle pulse
            end else begin
                counter_1ms <= counter_1ms + 1;
                tick_1ms_reg <= 1'b0;  // Pulse lasts only one cycle
            end
        end
    end
    
    // =========================================================================
    // 10ms Counter (derived from 1ms)
    // =========================================================================
    always @(posedge clk_50m or negedge reset_n) begin
        if (!reset_n) begin
            counter_10ms <= 0;
            tick_10ms_reg <= 1'b0;
        end else if (tick_1ms_reg) begin
            if (counter_10ms == 9) begin  // Count 10 x 1ms ticks
                counter_10ms <= 0;
                tick_10ms_reg <= 1'b1;
            end else begin
                counter_10ms <= counter_10ms + 1;
                tick_10ms_reg <= 1'b0;
            end
        end else begin
            tick_10ms_reg <= 1'b0;
        end
    end
    
    // =========================================================================
    // 100ms Counter (derived from 10ms)
    // =========================================================================
    always @(posedge clk_50m or negedge reset_n) begin
        if (!reset_n) begin
            counter_100ms <= 0;
            tick_100ms_reg <= 1'b0;
        end else if (tick_10ms_reg) begin
            if (counter_100ms == 9) begin  // Count 10 x 10ms ticks
                counter_100ms <= 0;
                tick_100ms_reg <= 1'b1;
            end else begin
                counter_100ms <= counter_100ms + 1;
                tick_100ms_reg <= 1'b0;
            end
        end else begin
            tick_100ms_reg <= 1'b0;
        end
    end
    
    // =========================================================================
    // 1s Counter (derived from 100ms)
    // =========================================================================
    always @(posedge clk_50m or negedge reset_n) begin
        if (!reset_n) begin
            counter_1s <= 0;
            tick_1s_reg <= 1'b0;
        end else if (tick_100ms_reg) begin
            if (counter_1s == 9) begin  // Count 10 x 100ms ticks
                counter_1s <= 0;
                tick_1s_reg <= 1'b1;
            end else begin
                counter_1s <= counter_1s + 1;
                tick_1s_reg <= 1'b0;
            end
        end else begin
            tick_1s_reg <= 1'b0;
        end
    end
    
    // =========================================================================
    // Output Assignments
    // =========================================================================
    assign tick_1ms   = tick_1ms_reg;
    assign tick_10ms  = tick_10ms_reg;
    assign tick_100ms = tick_100ms_reg;
    assign tick_1s    = tick_1s_reg;
    
endmodule