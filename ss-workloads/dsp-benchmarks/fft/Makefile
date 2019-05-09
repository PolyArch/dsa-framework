ifndef N
N=64
endif

DFG = compute0.dfg compute1.dfg compute_rec.dfg compute.dfg fine2.dfg fine1.dfg fine.dfg

SOFTBRAINS = sb-origin sb-new sb-multi sb-reconfig sb-256 sb-limited

SCALE=$(N)

MACROS=-D _N_=$(N)

include ../common/Makefile.inc

# sb-origin: The simplest origin Softbrain.
# sb-new: Softbrain with new instructions to remove control overhead.
sb-origin.o: %.o: %.cc compute.dfg.h fine2.dfg.h fine1.dfg.h
	$(CC) $< -o $@ -c $(MACROS) $(IFLAGS) -O3

sb-new.o: %.o: %.cc compute.dfg.h fine2.dfg.h fine1.dfg.h
	$(CC) $< -o $@ -c $(MACROS) $(IFLAGS) -O3
