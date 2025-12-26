module baud_rate_gen #(
    parameter integer CLK_FREQ = 50000000, // Default 50 MHz
    parameter integer BAUD_RATE = 9600     // Default 9600 Baud
)(
    input wire clk,
    input wire rst_n,
    output reg baud_tick
);

    // Calculate the number of clock cycles per bit
    localparam integer DIVISOR = CLK_FREQ / BAUD_RATE;
    
    // Counter to count clock cycles
    reg [31:0] counter;

    always @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            counter <= 0;
            baud_tick <= 1'b0;
        end else begin
            // Check if counter reached the limit
            if (counter >= (DIVISOR - 1)) begin
                counter <= 0;
                baud_tick <= 1'b1; // Generate pulse
            end else begin
                counter <= counter + 1;
                baud_tick <= 1'b0;
            end
        end
    end

endmodule