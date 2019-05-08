ifndef SS_TOOLS
$(error SS_TOOLS is undefined)
endif

KERN=md
KERN_R5=$(KERN)_r5
KERN_SB=$(KERN)_sb
ALG=knn

CFLAGS?=-O3 -Wall -Wno-unused-label

SRCS=$(KERN).c local_support.c ../../common/support.c
FILES=$(SRCS) $(KERN).h ../../common/support.h

##SB_Related
SB_SRCS=$(KERN_SB).c local_support.c ../../common/support.c
SB_FILES=$(SB_SRCS) $(KERN).h ../../common/support.h

SB_CPP=riscv64-unknown-elf-g++
#SB_CONFIG=$(SS_TOOLS)/configs/diannao_simd64_half.sbmodel
SB_CONFIG=$(SS_TOOLS)/configs/diannao_simd64.sbmodel

DFGS=md_sb.dfg
DFG_HEADERS=$(DFGS:.dfg=.dfg.h)

$(DFG_HEADERS): %.dfg.h: %.dfg
	$(SS_TOOLS)/bin/sb_sched $(SB_CONFIG) $<

$(KERN_SB): $(SB_FILES) ../../common/harness.c $(DFG_HEADERS) 
		$(SB_CPP) $(CFLAGS) -I../../common -o $(KERN_SB) $(SB_SRCS) ../../common/harness.c -DSB

run_sb: $(KERN_SB) input.data check.data
	SBCONFIG=$(SB_CONFIG) spike  --ic=128:4:64 --dc=128:4:64 --l2=1024:8:64  --extension=softbrain $(SS_TOOLS)/riscv64-unknown-elf/bin/pk  ./$(KERN_SB) input.data check.data

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
	rm -f $(KERN_SB) *.dfg.h
