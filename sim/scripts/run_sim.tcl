# TCL Script for ModelSim/Questa
vlib work
vlog -work work ../../hw/rtl/*.v
vlog -work work ../testbenches/*.v
vsim -voptargs=+acc work.tb_baud_generator
add wave -position insertpoint sim:/tb_baud_generator/*
run -all
