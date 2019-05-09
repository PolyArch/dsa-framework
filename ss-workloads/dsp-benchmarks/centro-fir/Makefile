ifndef N
N=1024
endif

ifndef M
M=37
endif

DFG = compute.dfg loader.dfg

DFG_HEADERS=$(DFG:.dfg=.h)

SCALE = $(N) $(M)

SOFTBRAINS = sb-new sb-origin sb-ideal sb-latency sb-limited

MACROS=-D_N_=$(N) -D_M_=$(M)

ifneq ($(N), 1024)
MACROS += -DLATENCY
endif

include ../common/Makefile.inc


# New instructions remove control overhead
sb-limited.o sb-origin.o sb-new.o sb-ideal.o sb-latency.o: %.o: %.cc compute.dfg.h loader.dfg.h
	$(CC) $< -o $@ -c $(MACROS) $(IFLAGS) -O3

