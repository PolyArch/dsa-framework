ifndef SBCONFIG
$(warning SBCONFIG is undefined, using default)
SBCONFIG=$(SS_TOOLS)/configs/diannao_simd64.sbmodel
endif

KERN=fft
KERN_SB=fft_sb
ALG=strided

DFG=compute.dfg
DFG_HEADERS=$(DFG:.dfg=.dfg.h)

$(DFG_HEADERS): %.dfg.h: %.dfg
	$(SS_TOOLS)/bin/sb_sched $(SBCONFIG) $< 

CC=riscv64-unknown-elf-g++

CFLAGS?=-O3 -Wall -Wno-unused-label

SRCS=$(KERN).c local_support.c ../../common/support.c
SRCS_SB=$(KERN_SB).c local_support.c ../../common/support.c
FILES=$(SRCS) $(KERN).h ../../common/support.h
FILES_SB=$(SRCS_SB) $(KERN).h ../../common/support.h $(DFG_HEADERS)

$(KERN): $(FILES) ../../common/harness.c
	$(CC) $(CFLAGS) -I../../common -o $(KERN) $(SRCS) ../../common/harness.c -lm

$(KERN_SB): $(FILES_SB) ../../common/harness.c
	$(CC) $(CFLAGS) -I../../common -o $(KERN_SB) $(SRCS_SB) ../../common/harness.c -lm -I ../../../common/include

run: $(KERN) input.data check.data
	SBCONFIG=$(SBCONFIG) \
	spike --ic=64:4:64 --dc=64:4:64 --l2=1024:8:64 --extension=softbrain $(SS_TOOLS)/riscv64-unknown-elf/bin/pk $(KERN)
	#./$(KERN) input.data check.data

run_sb: $(KERN_SB) input.data check.data
	SBCONFIG=$(SBCONFIG) \
	spike --ic=64:4:64 --dc=64:4:64 --l2=1024:8:64 --extension=softbrain $(SS_TOOLS)/riscv64-unknown-elf/bin/pk $(KERN_SB)

generate: $(FILES) generate.c
	$(CC) $(CFLAGS) -I../../common -o generate $(SRCS) generate.c -lm
	./generate

hls: $(KERN).c $(KERN).h
	vivado_hls hls.tcl

clean:
	rm -f $(KERN) $(KERN_SB) generate output.data $(DFG_HEADERS)
