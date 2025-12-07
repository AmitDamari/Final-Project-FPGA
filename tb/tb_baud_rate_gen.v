// =============================================================================
// COMPREHENSIVE TESTBENCH FOR BAUD RATE GENERATOR (Verilog-2001 Compatible)
// =============================================================================

`timescale 1ns/1ps

module tb_baud_rate_gen;

    // =========================================================================
    // TESTBENCH CONFIGURATION
    // =========================================================================
    
    parameter CLK_FREQ = 50000000;
    parameter BAUD_RATE = 9600;
    parameter CLK_PERIOD = 20;  // 50MHz = 20ns period
    parameter EXPECTED_PERIOD = (CLK_FREQ / BAUD_RATE) * CLK_PERIOD;
    
    // =========================================================================
    // SIGNAL DECLARATIONS (All at module level for Verilog-2001)
    // =========================================================================
    
    reg tb_clk;
    reg tb_rst_n;
    wire tb_baud_tick;
    
    // Test control
    integer test_number;
    integer error_count;
    integer total_tests;
    
    // Pulse measurement
    integer pulse_count;
    time first_pulse_time;
    time second_pulse_time;
    time last_pulse_time;
    time current_pulse_time;
    
    // For period calculation
    integer current_period;
    integer min_period;
    integer max_period;
    integer total_period;
    integer avg_period;
    
    // Loop counters and temporaries
    integer i;
    integer start_count;
    time start_time;
    integer measured_period;
    integer tolerance;
    integer timing_error;
    integer num_pulses;
    integer expected_pulses;
    time rise_time;
    integer pulse_width;
    integer pulse_count_before;
    integer pulse_count_after;
    
    // =========================================================================
    // DEVICE UNDER TEST
    // =========================================================================
    
    baud_rate_gen #(
        .CLK_FREQ(CLK_FREQ),
        .BAUD_RATE(BAUD_RATE)
    ) dut (
        .clk(tb_clk),
        .rst_n(tb_rst_n),
        .baud_tick(tb_baud_tick)
    );
    
    // =========================================================================
    // CLOCK GENERATION
    // =========================================================================
    
    initial begin
        tb_clk = 0;
        forever #(CLK_PERIOD/2) tb_clk = ~tb_clk;
    end
    
    // =========================================================================
    // PULSE MONITORING
    // =========================================================================
    
    always @(posedge tb_baud_tick) begin
        pulse_count = pulse_count + 1;
        current_pulse_time = $time;
        
        if (pulse_count == 1) begin
            first_pulse_time = $time;
            last_pulse_time = $time;
        end else if (pulse_count == 2) begin
            second_pulse_time = $time;
            current_period = $time - last_pulse_time;
            min_period = current_period;
            max_period = current_period;
            total_period = current_period;
            last_pulse_time = $time;
        end else begin
            current_period = $time - last_pulse_time;
            if (current_period < min_period) min_period = current_period;
            if (current_period > max_period) max_period = current_period;
            total_period = total_period + current_period;
            last_pulse_time = $time;
        end
    end
    
    // =========================================================================
    // REUSABLE TASKS
    // =========================================================================
    
    task init_test_vars;
        begin
            pulse_count = 0;
            first_pulse_time = 0;
            second_pulse_time = 0;
            last_pulse_time = 0;
            current_pulse_time = 0;
            min_period = 0;
            max_period = 0;
            total_period = 0;
            avg_period = 0;
        end
    endtask
    
    task apply_reset;
        input integer duration;
        begin
            $display("[%0t] Applying reset for %0d ns", $time, duration);
            tb_rst_n = 0;
            #duration;
            tb_rst_n = 1;
            $display("[%0t] Reset released", $time);
        end
    endtask
    
    task wait_for_pulses;
        input integer num_pulses_to_wait;
        input integer timeout_ns;
        begin
            start_count = pulse_count;
            start_time = $time;
            $display("[%0t] Waiting for %0d pulses...", $time, num_pulses_to_wait);
            
            while ((pulse_count - start_count) < num_pulses_to_wait) begin
                #(CLK_PERIOD);
                if (($time - start_time) > timeout_ns) begin
                    $display("ERROR: Timeout waiting for pulses!");
                    error_count = error_count + 1;
                    disable wait_for_pulses;
                end
            end
            
            $display("[%0t] Received %0d pulses", $time, num_pulses_to_wait);
        end
    endtask
    
    task verify_timing;
        input integer expected_period_ns;
        input integer tolerance_percent;
        begin
            $display("----------------------------------------");
            $display("TIMING VERIFICATION:");
            
            if (pulse_count < 2) begin
                $display("  ERROR: Not enough pulses to verify timing");
                $display("  Pulse count: %0d", pulse_count);
                error_count = error_count + 1;
                $display("----------------------------------------");
            end else begin
                measured_period = second_pulse_time - first_pulse_time;
                tolerance = (expected_period_ns * tolerance_percent) / 100;
                timing_error = measured_period - expected_period_ns;
                
                $display("  Expected period: %0d ns", expected_period_ns);
                $display("  Measured period: %0d ns", measured_period);
                $display("  Error: %0d ns", timing_error);
                $display("  Tolerance: +/- %0d ns (%0d%%)", tolerance, tolerance_percent);
                
                if ((timing_error > tolerance) || (timing_error < -tolerance)) begin
                    $display("  Result: FAIL - Timing out of tolerance");
                    error_count = error_count + 1;
                end else begin
                    $display("  Result: PASS");
                end
                $display("----------------------------------------");
            end
        end
    endtask
    
    task display_statistics;
        input integer expected_pulses_count;
        begin
            if (pulse_count > 1) begin
                avg_period = total_period / (pulse_count - 1);
            end
            
            $display("========================================");
            $display("PULSE STATISTICS:");
            $display("  Total pulses received: %0d", pulse_count);
            $display("  Expected pulses: %0d", expected_pulses_count);
            $display("  Expected period: %0d ns", EXPECTED_PERIOD);
            
            if (pulse_count > 1) begin
                $display("  Average period: %0d ns", avg_period);
                $display("  Min period: %0d ns", min_period);
                $display("  Max period: %0d ns", max_period);
                $display("  Jitter: %0d ns", max_period - min_period);
            end
            $display("========================================");
        end
    endtask
    
    // =========================================================================
    // TEST CASES
    // =========================================================================
    
    task test_basic_functionality;
        begin
            test_number = 1;
            $display("\n========================================");
            $display("TEST 1: BASIC FUNCTIONALITY");
            $display("========================================");
            
            init_test_vars();
            apply_reset(100);
            
            num_pulses = 10;
            wait_for_pulses(num_pulses, EXPECTED_PERIOD * 15);
            verify_timing(EXPECTED_PERIOD, 1);
            display_statistics(num_pulses);
            
            total_tests = total_tests + 1;
        end
    endtask
    
    task test_reset_during_operation;
        begin
            test_number = 2;
            $display("\n========================================");
            $display("TEST 2: RESET DURING OPERATION");
            $display("========================================");
            
            init_test_vars();
            apply_reset(100);
            
            wait_for_pulses(3, EXPECTED_PERIOD * 5);
            
            $display("[%0t] Asserting reset during operation...", $time);
            tb_rst_n = 0;
            #200;
            
            if (tb_baud_tick !== 0) begin
                $display("ERROR: baud_tick should be 0 during reset");
                error_count = error_count + 1;
            end else begin
                $display("PASS: baud_tick correctly held low during reset");
            end
            
            tb_rst_n = 1;
            $display("[%0t] Reset released, resuming operation", $time);
            
            init_test_vars();
            wait_for_pulses(5, EXPECTED_PERIOD * 8);
            verify_timing(EXPECTED_PERIOD, 1);
            
            total_tests = total_tests + 1;
        end
    endtask
    
    task test_multiple_resets;
        begin
            test_number = 3;
            $display("\n========================================");
            $display("TEST 3: MULTIPLE SHORT RESETS");
            $display("========================================");
            
            for (i = 0; i < 5; i = i + 1) begin
                $display("\n--- Reset iteration %0d ---", i+1);
                init_test_vars();
                apply_reset(50);
                wait_for_pulses(3, EXPECTED_PERIOD * 5);
            end
            
            $display("PASS: Module recovered from multiple resets");
            total_tests = total_tests + 1;
        end
    endtask
  
    task test_pulse_width;
        begin
            test_number = 4;
            $display("\n========================================");
            $display("TEST 4: PULSE WIDTH VERIFICATION");
            $display("========================================");
            
            init_test_vars();
            apply_reset(100);
            
            @(posedge tb_baud_tick);
            rise_time = $time;
            
            @(negedge tb_baud_tick);
            pulse_width = $time - rise_time;
            
            $display("Pulse width: %0d ns (%0d clock cycles)", 
                     pulse_width, pulse_width/CLK_PERIOD);
            
            if (pulse_width == CLK_PERIOD) begin
                $display("PASS: Pulse width is exactly 1 clock cycle");
            end else begin
                $display("ERROR: Pulse width should be 1 clock cycle (got %0d cycles)", 
                         pulse_width/CLK_PERIOD);
                error_count = error_count + 1;
            end
            
            total_tests = total_tests + 1;
        end
    endtask
    
    task test_no_pulses_during_reset;
        begin
            test_number = 5;
            $display("\n========================================");
            $display("TEST 5: NO PULSES DURING RESET");
            $display("========================================");
            
            init_test_vars();
            
            tb_rst_n = 0;
            pulse_count_before = pulse_count;
            
            #10000;
            
            pulse_count_after = pulse_count;
            
            if (pulse_count_after != pulse_count_before) begin
                $display("ERROR: Received %0d pulses during reset", 
                         pulse_count_after - pulse_count_before);
                error_count = error_count + 1;
            end else begin
                $display("PASS: No pulses generated during reset");
            end
            
            tb_rst_n = 1;
            total_tests = total_tests + 1;
        end
    endtask
    
    task test_long_duration;
        begin
            test_number = 6;
            $display("\n========================================");
            $display("TEST 6: LONG DURATION STABILITY");
            $display("========================================");
            
            init_test_vars();
            apply_reset(100);
            
            num_pulses = 50;
            $display("Running extended test with %0d pulses...", num_pulses);
            wait_for_pulses(num_pulses, EXPECTED_PERIOD * (num_pulses + 5));
            
            verify_timing(EXPECTED_PERIOD, 1);
            display_statistics(num_pulses);
            
            if ((max_period - min_period) > (EXPECTED_PERIOD / 100)) begin
                $display("WARNING: Jitter exceeds 1%% of period");
            end else begin
                $display("PASS: Jitter within acceptable limits");
            end
            
            total_tests = total_tests + 1;
        end
    endtask
    
    // =========================================================================
    // MAIN TEST SEQUENCE
    // =========================================================================
    
    initial begin
        $dumpfile("baud_comprehensive.vcd");
        $dumpvars(0, tb_baud_rate_gen);
        
        test_number = 0;
        error_count = 0;
        total_tests = 0;
        
        $display("\n=============================================================================");
        $display("COMPREHENSIVE BAUD RATE GENERATOR TESTBENCH");
        $display("=============================================================================");
        $display("Configuration:");
        $display("  CLK_FREQ:    %0d Hz", CLK_FREQ);
        $display("  BAUD_RATE:   %0d bps", BAUD_RATE);
        $display("  CLK_PERIOD:  %0d ns", CLK_PERIOD);
        $display("  DIVISOR:     %0d", CLK_FREQ / BAUD_RATE);
        $display("  BIT_PERIOD:  %0d ns", EXPECTED_PERIOD);
        $display("=============================================================================");
        
        tb_rst_n = 0;
        init_test_vars();
        
        #100;
        
        // Run all tests
        test_basic_functionality();
        test_reset_during_operation();
        test_multiple_resets();
        test_pulse_width();
        test_no_pulses_during_reset();
        test_long_duration();
        
        // Final summary
        #1000;
        $display("\n=============================================================================");
        $display("TEST SUMMARY");
        $display("=============================================================================");
        $display("Total tests run:    %0d", total_tests);
        $display("Errors detected:    %0d", error_count);
        
        if (error_count == 0) begin
            $display("\n*** ALL TESTS PASSED ***");
        end else begin
            $display("\n*** TESTS FAILED (%0d errors) ***", error_count);
        end
        $display("=============================================================================\n");
        
        $finish;
    end
    
    // Timeout watchdog
    initial begin
        #100000000;
        $display("\nERROR: Simulation timeout after 100ms");
        $finish;
    end

endmodule