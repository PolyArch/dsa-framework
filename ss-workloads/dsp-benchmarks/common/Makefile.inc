ifndef SBCONFIG
$(warning SBCONFIG is undefined, using default)
SBCONFIG=$(SS)"/ss-scheduler/configs/revel-1x1.sbmodel"
endif

ifndef OMP_NUM_THREADS
OMP_NUM_THREADS=1
endif

ifndef NUM_PTHREADS
NUM_PTHREADS=1
endif

ifndef FIFO_DEPTH
FIFO_DEPTH=15
endif

PYTHON = env python3

DFG_HEADERS = $(DFG:.dfg=.dfg.h)

$(DFG_HEADERS): %.dfg.h: %.dfg
	$(SS_TOOLS)/bin/ss_sched -a sa --max-iters 20000 -d $(FIFO_DEPTH) --verbose $(SBCONFIG) $< 

IFLAGS=-I../../common/include -I../../../ss-tools/include -I../common/ -I$(SS_TOOLS)/include/ss-intrin
INTELI=-I/opt/intel/include -I/opt/intel/mkl/include/
#CFLAGS=-O3 -std=c++17
CFLAGS=-Ofast -std=c++17
LMKL=-L"/opt/intel/mkl/lib" -L"/opt/intel/lib/intel64" -lmkl_intel_lp64 -lmkl_intel_thread -lmkl_core -liomp5 -lpthread -ldl -lm

CC=riscv64-unknown-elf-g++

BENCHMARKS = optimized
OBJS = $(BENCHMARKS:=.o)
LOGS = $(SOFTBRAINS:=.log)
EXEC = $(BENCHMARKS:=.exe) $(SOFTBRAINS:=.exe)
THREAD_MACROS = -DNUM_PTHREADS=$(NUM_PTHREADS) -DOMP_NUM_THREADS=$(OMP_NUM_THREADS)

MAT_DFG = ../common/mmv.dfg ../common/mmvc.dfg ../common/suboutercx2.dfg ../common/subouterx2.dfg ../common/vcmm.dfg ../common/vmm.dfg
MAT_HEADS = $(MAT_DFG:.dfg=.dfg.h)

$(MAT_HEADS): %.dfg.h: %.dfg
	$(SS_TOOLS)/bin/sb_sched -a sa --max-iters 20000 -d $(FIFO_DEPTH) --verbose $(SBCONFIG) $< 

matutils: $(MAT_HEADS)

input.data ref.data: gen.py
	$(PYTHON) $< $(SCALE) > gen.log

$(OBJS): %.o: %.cc
	$(CC) $< -o $@ -c $(MACROS) $(IFLAGS) $(CFLAGS)

physical.exe: main.cc optimized.cc
	g++ $^ -o $@ $(MACROS) $(IFLAGS) $(CFLAGS)

physical.log mkl.log: %.log: %.exe input.data ref.data
	./$^ 2>&1 | tee $@

mkl.exe: mkl.cc
	/opt/intel/bin/icc $^ -o $@ $(LMKL) $(CFLAGS) $(MACROS) $(THREAD_MACROS) $(IFLAGS) $(INTELI)

ooo.log: optimized.exe input.data ref.data
	SBCONFIG=$(SBCONFIG) \
	gem5.opt ~/ss-stack/gem5/configs/example/se.py --cpu-type=detailed --l1d_size=64kB --l1i_size=16kB --l2_size=1024kB --caches --l2cache --cmd=./$< \
	| tee $@

$(LOGS): %.log: %.exe input.data ref.data
	SBCONFIG=$(SBCONFIG) \
	FU_FIFO_LEN=$(FIFO_DEPTH) \
	gem5.opt ~/ss-stack/gem5/configs/example/se.py --cpu-type=MinorCPU \
	--l1d_size=2048kB --l1d_assoc=32 --l1i_size=16kB --l2_size=1024kB --caches --cmd=./$< \
	| tee $@
	
$(EXEC): %.exe: %.o main.cc
	$(CC) main.cc $(basename $@).o -o $@ -lm $(CFLAGS) $(MACROS) $(IFLAGS)

cleandata:
	rm -f input.data ref.data

cleandfg:
	rm -rf *.dfg.h
	rm -rf sched
	rm -rf stats gams m5out viz verif

clean: cleandata
	rm -f *.o *.log *.exe

cleanmat:
	rm -rf ../common/*.dfg.h

ultraclean: clean cleandfg
	rm -f *.csv
	rm -rf log_*
