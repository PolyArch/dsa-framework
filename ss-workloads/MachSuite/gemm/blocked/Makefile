ifndef SS_TOOLS
$(error SS_TOOLS is undefined)
endif

RUN?=sim

KERN=gemm
KERN_R5=$(KERN)_r5
KERN_SB=$(KERN)_sb
ALG=blocked

CFLAGS?=-O3 -Wall -Wno-unused-label --std=c++11

SRCS=$(KERN).c local_support.c ../../common/support.c
FILES=$(SRCS) $(KERN).h ../../common/support.h

##SB_Related
SB_SRCS=$(KERN_SB).c local_support.c ../../common/support.c
SB_FILES=$(SB_SRCS) $(KERN).h ../../common/support.h

SB_CPP=riscv64-unknown-elf-g++
CPP=g++
#SB_CONFIG=$(SS_TOOLS)/configs/diannao_simd64_half.sbmodel
SB_CONFIG=$(SS_TOOLS)/configs/diannao_simd64.sbmodel
#SB_CONFIG=$(SS_TOOLS)/configs/isca2017.sbmodel

DFGS=mm_sb.dfg
DFG_HEADERS=$(DFGS:.dfg=.dfg.h)

all: $(RUN)


$(DFG_HEADERS): %.dfg.h: %.dfg
ifeq ($(RUN), sim)
	$(SS_TOOLS)/bin/sb_sched $(SB_CONFIG) $<
else
	$(SS_TOOLS)/bin/sb_dfg_emu none.cfg $<
endif

$(KERN_SB): $(SB_FILES) ../../common/harness.c $(DFG_HEADERS) 
	$(SB_CPP) $(CFLAGS) -I../../common -o $(KERN_SB) $(SB_SRCS) ../../common/harness.c -DSB

sim: $(SB_FILES) ../../common/harness.c $(DFG_HEADERS) 
	$(SB_CPP) $(CFLAGS) -I../../common -o $(KERN_SB) $(SB_SRCS) ../../common/harness.c -DSB -DRUN

emu: $(SB_FILES) ../../common/harness.c $(DFG_HEADERS)
	$(CPP) $(CFLAGS) -DSB $(SB_SRCS) ../../common/harness.c -I../../common -lsoftbrain-emu -I$(SS_TOOLS)/include/softbrain-lib -L$(SS_TOOLS)/lib -o $(KERN_SB) 

run_sb: $(KERN_SB) input.data check.data
	SBCONFIG=$(SB_CONFIG) spike  --ic=128:4:64 --dc=128:4:64 --l2=1024:8:64  --extension=softbrain $(SS_TOOLS)/riscv64-unknown-elf/bin/pk  ./$(KERN_SB) input.data check.data

run_emu_sb: $(KERN_SB) input.data check.data
	./$(KERN_SB) input.data check.data

run_r5: $(KERN_R5) input.data check.data
	SBCONFIG=$(SB_CONFIG) spike  --ic=128:4:64 --dc=128:4:64 --l2=1024:8:64  --extension=softbrain $(SS_TOOLS)/riscv64-unknown-elf/bin/pk ./$(KERN_R5) input.data check.data


$(KERN_R5): $(FILES) ../../common/harness.c
	$(SB_CPP) $(CFLAGS) -I../../common -o $(KERN_R5) $(SRCS) ../../common/harness.c

$(KERN): $(FILES) ../../common/harness.c
	$(CC) $(CFLAGS) -I../../common -o $(KERN) $(SRCS) ../../common/harness.c

run: $(KERN) input.data check.data
	./$(KERN) input.data check.data

generate: $(FILES) generate.c
	$(CC) $(CFLAGS) -I../../common -o generate $(SRCS) generate.c
	./generate

hls: $(KERN).c $(KERN).h
	vivado_hls hls.tcl

clean:
	rm -f $(KERN) generate output.data
	rm -rf *.dfg.h $(KERN_SB)
