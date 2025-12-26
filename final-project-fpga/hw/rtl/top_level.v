module top_level (
    input  wire        CLOCK_50,
    input  wire [3:0]  KEY,
    input  wire [9:0]  SW,
    output wire [9:0]  LEDR
);

    // Force ALL LEDs ON
    assign LEDR = 10'b1111111111;

endmodule