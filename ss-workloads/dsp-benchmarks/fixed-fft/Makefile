ifndef N
N=64
endif

DFG = compute0.dfg compute1.dfg compute_rec.dfg compute.dfg manager.dfg fine2.dfg fine1.dfg manager8.dfg fine0.dfg

SOFTBRAINS = sb-origin sb-new sb-multi sb-reconfig

SCALE=$(N)

MACROS=-D _N_=$(N)

include ../common/Makefile.inc

# sb-origin: The simplest origin Softbrain.
# sb-new: Softbrain with new instructions to remove control overhead.
# sb-scr: All the data will go through the spad instead of memory. TOO AGGRESSIVE, crashes on large scale.
sb-reconfig.o sb-origin.o: %.o: %.cc compute0.dfg.h compute1.dfg.h fine0.dfg.h
	$(CC) $< -o $@ -c $(MACROS) $(IFLAGS) -O3

sb-multi.o sb-new.o: %.o: %.cc compute.dfg.h fine2.dfg.h fine1.dfg.h fine0.dfg.h
	$(CC) $< -o $@ -c $(MACROS) $(IFLAGS) -O3

