ifndef N
N=12
endif


DFG = vmc.dfg vv.dfg mvc.dfg vvc.dfg vm.dfg aplygvs.dfg hhr.dfg fused.dfg finalize.dfg lmm2x2.dfg mmv.dfg vcmm.dfg mmvc.dfg vmm.dfg

SCALE=$(N)

MACROS=-D _N_=$(N)

SOFTBRAINS = sb-origin sb-new sb-access sb-conc sb-limited

include ../common/Makefile.inc

sb-origin.o sb-access.o: %.o: %.cc vmc.dfg.h vv.dfg.h mvc.dfg.h vvc.dfg.h vm.dfg.h aplygvs.dfg.h finalize.dfg.h
	$(CC) $< -o $@ -c $(MACROS) $(IFLAGS) -O3

sb-conc.o: %.o: %.cc  sb-new.cc hhr.dfg.h fused.dfg.h aplygvs.dfg.h finalize.dfg.h
	$(CC) $< -o $@ -c $(MACROS) $(IFLAGS) -O3

sb-new.o: sb-new.cc hhr.dfg.h fused.dfg.h aplygvs.dfg.h finalize.dfg.h
	$(CC) $< -o $@ -c $(MACROS) $(IFLAGS) -O3

sb-limited.o: sb-limited.cc hhr.dfg.h fused.dfg.h aplygvs.dfg.h finalize.dfg.h
	$(CC) $< -o $@ -c $(MACROS) $(IFLAGS) -O3
