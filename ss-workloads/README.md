Stream Specialization Workloads
===============================

Runing an example workload through simulation:
----------------------------------------------

Go into `dsp-benchmarks` and attempt a build.

For each workloads in this folder, you can type `make sb-\*.log` to simulate the corresponding workloads.

* origin: This is for the very original Softbrain's performance.
* new: This is for our new architecture REVEL's performance.
* access: This is to see the performance effects of the specialized data streams.
* conc: This is to see the performance effects of allowing multiple DFGs on the CGRA (and access).
* hetro: This is to see the performance effects of adding temporal region (accompanied with both access and conc).

For example:

````
cd dsp-benchmarks/cholesky
make sb-new.log
````
