export SBCONFIG=$SS_TOOLS/configs/revel.sbmodel
#spike --ic=64:4:64 --dc=64:4:64 --l2=1024:8:64 --extension=softbrain $SS_TOOLS/riscv64-unknown-elf/bin/pk dotp
#spike --extension=softbrain $SS_TOOLS/riscv64-unknown-elf/bin/pk dotp 
gem5.opt $SS_STACK/gem5/configs/example/se.py --cpu-type=MinorCPU --l1d_size=64kB --l1i_size=16kB --l2_size=1024kB --caches --cmd=bin/dotp
