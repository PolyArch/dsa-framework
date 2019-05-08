ifndef SBCONFIG
$(warning SBCONFIG is undefined, using default)
SBCONFIG=$(SS)/ss-scheduler/configs/revel-1x2.sbmodel
endif

TESTN=$(wildcard fix*.c)
TESTALL=$(wildcard *.c)
TESTC=$(filter-out $(TESTN), $(TESTALL))

TESTSN=$(patsubst %.c,bin/%,$(TESTN))
TESTS_SMALL=$(patsubst %.c,bin/%_36,$(TESTC))
TESTS_MEDIUM=$(patsubst %.c,bin/%_128,$(TESTC))
TESTS_LARGE=$(patsubst %.c,bin/%_2048,$(TESTC))

TESTS=$(TESTS_SMALL) $(TESTS_MEDIUM) $(TESTS_LARGE) $(TESTSN) 

all: bin $(TESTS) 

CPP=riscv64-unknown-elf-g++
#CPP=riscv64-unknown-linux-gnu-g++ 

DFGS = $(wildcard *.dfg)
DFG_HEADERS = $(DFGS:.dfg=.dfg.h)
HEADERS = testing.h check.h
IFLAGS = -I$(SS)/ss-tools/include/ss-intrin
#${info $(DFG_HEADERS)}

bin:
	mkdir -p bin

$(DFG_HEADERS): %.dfg.h: %.dfg
	$(RISCV)/bin/ss_sched -a sa $(SBCONFIG) $<

OPT?=-O3
CFLAGS=$(OPT) -g -ggdb -gdwarf-3 -static --std=c++11
#--std=c++11 

$(TESTSN):  bin/% : %.c $(DFG_HEADERS) $(HEADERS) bin
	 $(CPP) $< $(LIB) $(CFLAGS) $(IFLAGS) -o $@ -DASIZE=16

$(TESTS_SMALL): bin/%_36 : %.c  $(DFG_HEADERS) $(HEADERS) bin
	 $(CPP) $< $(LIB) $(CFLAGS) $(IFLAGS) -o $@ -DASIZE=12
$(TESTS_MEDIUM): bin/%_128 : %.c  $(DFG_HEADERS)  $(HEADERS) bin
	 $(CPP) $< $(LIB) $(CFLAGS) $(IFLAGS) -o $@ -DASIZE=500
$(TESTS_LARGE): bin/%_2048 : %.c  $(DFG_HEADERS)  $(HEADERS) bin
	 $(CPP) $< $(LIB) $(CFLAGS) $(IFLAGS) -o $@ -DASIZE=2048


clean:
	rm -f $(TESTS) $(DFG_HEADERS) *.s *.o
	rm -rf m5out viz verif stats gams bin
