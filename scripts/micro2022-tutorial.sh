###########################################
# Software Stack and Programming Interface
###########################################
cd $SS/dsa-apps/demo

## Run gem5 simulation
./run.sh ss-mv.out
./run.sh ss-crs.out

## Compile RISC-V Binary
make ultraclean
make ss-mv.riscv
make ss-crs.riscv

## DFG Visualization
ss_sched spmv_0_1_1.dfg -f
neato -Tpng -overlap=false -Gepsilon=.0001 -o spmv_0_1_1.png spmv_0_1_1.gv

## Scheduled DFG-ADG Visualization
ss_sched spmv_0_1_1.dfg ../adg/Mesh7x5-Simple64-Full7I5O.json -f
dot -Tpng -o spmv_0_1_1.png spmv_0_1_1.gv

###########################################
# Hardware Stack and RTL Simulation
###########################################
cd $SS/chipyard

## Compile Verilator RTL Simulation File
make -C sims/verilator CONFIG=MeshDSARocketConfig

## ADG Visualization
python3 $SS/scripts/adg_visualization.py generators/dsagen2/adg/Mesh7x5-Simple64-Full7I5O.json

## Compile all unit tests
cd $SS/chipyard/generators/dsagen2
make ADG=$SS/chipyard/generators/dsagen2/adg/Mesh7x5-Simple64-Full7I5O.json compile-microbench

## Run all unit tests
cd $SS/chipyard
make -C sims/verilator CONFIG=MeshDSARocketConfig ss-run

## Run mv and crs
make -C sims/verilator CONFIG=MeshDSARocketConfig BINARY=$SS/dsa-apps/demo/ss-mv.riscv
make -C sims/verilator CONFIG=MeshDSARocketConfig BINARY=$SS/dsa-apps/demo/ss-crs.riscv

###########################################
# Visualization and Design Space Exploration
###########################################
cd $SS/dsa-apps/demo

## DSE Commands
python3 extract.py
ss_sched dfgs.list ../adg/Mesh7x5-Simple64-Full7I5O.json -x -f -m 200 --dse-timeout=500

## Post-DSE Visualization
python3 $SS/scripts/adg_visualization.py viz/prunned_sched-adg.json
