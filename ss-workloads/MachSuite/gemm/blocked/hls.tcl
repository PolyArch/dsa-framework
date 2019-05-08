open_project gemm_syn

add_files gemm.c
add_files input.data
add_files check.data
add_files -tb ../../common/harness.c

set_top bbgemm

open_solution -reset solution
set_part virtex7
create_clock -period 10
source ./gemm_dir
csynth_design
cosim_design -rtl verilog -tool modelsim -trace_level all

exit
