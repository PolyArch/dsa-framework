ifndef N
N=12
endif

SOFTBRAINS = sb-origin sb-new sb-access sb-hetro sb-conc sb-latency

DFG = \
  nmlz.dfg finalize.dfg norm.dfg hhr.dfg fused.dfg vcmm.dfg loader.dfg \
  rdfg.dfg qdfg.dfg l_rdfg.dfg l_qdfg.dfg

SCALE=$(N)

MACROS=-D_N_=$(N)

include ../common/Makefile.inc

SOFTBRAINS = sb-origin

# original softbrain
sb-origin.o sb-access.o: %.o: %.cc finalize.dfg.h nmlz.dfg.h norm.dfg.h vcmm.dfg.h loader.dfg.h
	$(CC) $< -o $@ -c $(MACROS) $(IFLAGS) -O3

sb-hetro.o sb-new.o sb-conc.o: %.o: %.cc rdfg.dfg.h qdfg.dfg.h qdfg.dfg.h loader.dfg.h
	$(CC) $< -o $@ -c $(MACROS) $(IFLAGS) -O3

sb-latency.o: %.o: %.cc l_rdfg.dfg.h l_qdfg.dfg.h qdfg.dfg.h loader.dfg.h
	$(CC) $< -o $@ -c $(MACROS) $(IFLAGS) -O3

