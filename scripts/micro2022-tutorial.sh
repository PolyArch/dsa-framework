###########################################
# Software Stack and Programming Interface
###########################################
cd $SS/dsa-apps/sdk/compiled

## Run gem5 simulation
make vecadd.ll
make ss-vecadd.ll
make ss-vecadd.s
make ss-vecadd.out
./run.sh ss-vecadd.out

## DFG Visualization
ss_sched vecadd_0_1.dfg -f
neato -Tpng -overlap=false -Gepsilon=.0001 -o vecadd_0_1_1.png vecadd_0_1.gv

## ADG Visualization
python3 $SS/scripts/adg_visualization.py generators/dsagen2/adg/Mesh7x5-Simple64-Full7I5O.json

## Scheduled DFG-ADG Visualization
ss_sched vecadd_0_1.dfg ../adg/Mesh7x5-Simple64-Full7I5O.json -f
dot -Tpng -o vecadd_0_1.png vecadd_0_1.gv

## Demo a MV-Suite
cd $SS/dsa-apps/demo
./run.sh ss-mv.out
./run.sh ss-crs.out

## Compile RISC-V Binary
make ultraclean
make ss-mv.riscv
make ss-crs.riscv

###########################################
# Hardware Stack and RTL Simulation
###########################################
cd $SS/chipyard

## Compile Verilator RTL Simulation File
make -C sims/verilator CONFIG=MeshDSARocketConfig

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
