ifndef SBCONFIG
$(warning SBCONFIG is undefined, using default)
SBCONFIG=$(SS_TOOLS)/configs/revel.sbmodel
endif

TESTS=dotp #red
TESTC=$(TESTS:=.c)

all: $(TESTS) bin

#CPP=riscv32-unknown-elf-g++
CPP=riscv64-unknown-elf-g++

DFGS=dot.dfg dot_acc.dfg #red.dfg

DFG_HEADERS=$(DFGS:.dfg=.dfg.h)

#${info $(DFGS)}
#${info $(DFG_HEADERS)}

$(DFG_HEADERS): %.dfg.h: %.dfg
	$(SS_TOOLS)/bin/sb_sched --verbose $(SBCONFIG) $< 

OPT?=-O3
CFLAGS=$(OPT) -g -ggdb -gdwarf-3 
#--std=c++11 

bin: 
	mkdir -p $@

$(TESTS): % :%.c $(DFG_HEADERS) bin
	 $(CPP) $< $(LIB) $(CFLAGS) -static -o bin/$@

clean:
	rm -f $(TESTS) $(DFG_HEADERS) *.s *.o
