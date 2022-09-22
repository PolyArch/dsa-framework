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

###########################################
# Hardware Stack and RTL Simulation
###########################################
cd $SS/chipyard

## Compile Verilator RTL Simulation File
make -C sims/verilator CONFIG=MeshDSARocketConfig

## Visualization
python3 $SS/scripts/adg_visualization.py generators/dsagen2/adg/Mesh7x5-Simple64-Full7I5O.json
cp $SS/chipyard/generators/dsagen2/adg/Mesh7x5-Simple64-Full7I5O.html /root/dsa-share

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

## DFG Visualization
ss_sched dfg_name -f
dot -Tpng -o dfg_name.png dfg_name.gv
neato -Tpng -overlap=false -Gepsilon=.0001 -o dfg_name.png dfg_name.gv

## ADG Visualization
python3 $SS/scripts/adg_visualization.py adg_name.json

## Scheduled DFG-ADG Visualization
ss_sched dfg_name adg_name -f
dot -Tpng -o dfg_name.png dfg_name.gv

## DSE Commands
cd $SS/dsa-apps/demo
python3 extract.py
ss_sched dfgs.list ../adg/Mesh7x5-Simple64-Full7I5O.json -x -f -m 200 --dse-timeout=500

## Post-DSE Visualization
python3 $SS/scripts/adg_visualization.py viz/prunned_sched-adg.json
