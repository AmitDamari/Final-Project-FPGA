module top_level (
    input  wire        CLOCK_50,
    input  wire [3:0]  KEY,
    input  wire [9:0]  SW,
    output wire [9:0]  LEDR,
    output wire [6:0]  HEX0,
    output wire [6:0]  HEX1,
    output wire [6:0]  HEX2,
    output wire [6:0]  HEX3,
    output wire [6:0]  HEX4,
    output wire [6:0]  HEX5
);

    //===========================================
    // TEST: Switches control LEDs directly
    //===========================================
    assign LEDR = SW;
    
    //===========================================
    // TEST: Display "Hello" on HEX (active LOW)
    //===========================================
    // HEX segment mapping:
    //        0
    //       ---
    //    5 |   | 1
    //       -6-
    //    4 |   | 2
    //       ---
    //        3
    
    assign HEX0 = 7'b1111111;  // Blank (all OFF)
    assign HEX1 = 7'b1000000;  // "O" (0)
    assign HEX2 = 7'b0001110;  // "L"
    assign HEX3 = 7'b0001110;  // "L"
    assign HEX4 = 7'b0000110;  // "E"
    assign HEX5 = 7'b1001000;  // "H"

endmodule