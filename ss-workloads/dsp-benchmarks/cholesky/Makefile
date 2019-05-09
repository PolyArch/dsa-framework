
ifndef N
N=12
endif

DFG = compute.dfg writeback.dfg multi2.dfg

SCALE=$(N)

MACROS=-D_N_=$(N)

SOFTBRAINS = sb-origin sb-new sb-latency sb-access sb-mask sb-lmask sb-limited sb-idiom

include ../common/Makefile.inc

# The cholesky implemented in origin Softbrain. The comparison baseline.
sb-origin.o sb-access.o: %.o: %.cc compute.dfg.h writeback.dfg.h
	$(CC) $< -o $@ -c $(MACROS) $(IFLAGS) -O3

# On REVEL, using multi-DFG-single-CGRA tech gives pretty promissing
# acceleration.
sb-latency.o: %.o: %.cc multi2.dfg.h
	$(CC) $< -o $@ -c $(MACROS) $(IFLAGS) -O3

sb-new.o sb-mask.o sb-lmask.o sb-limited.o sb-idiom.o: %.o: %.cc multi2.dfg.h
	$(CC) $< -o $@ -c $(MACROS) $(IFLAGS) -O3

