ifndef N
N=12
endif

SOFTBRAINS = sb-origin sb-new

DFG = nmlz.dfg finalize.dfg norm.dfg hhr.dfg fused.dfg vcmm.dfg
SCALE=$(N)

MACROS=-D_N_=$(N)

include ../common/Makefile.inc

SOFTBRAINS = sb-origin

# original softbrain
sb-origin.o: sb-origin.cc finalize.dfg.h nmlz.dfg.h norm.dfg.h vcmm.dfg.h
	$(CC) $< -o $@ -c $(MACROS) $(IFLAGS) -O3

sb-new.o: sb-new.cc hhr.dfg.h fused.dfg.h
	$(CC) $< -o $@ -c $(MACROS) $(IFLAGS) -O3

