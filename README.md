Stream Specialization Stack
===========================

A compiler, simulator, and modeling envorinment for stream-dataflow-based architectures.

The following are the main submodules:
* Implementation of stream-dataflow++ within RISCV *(riscv-opcodes)*
* Gem5 Simulator *(gem5)*
* Modified RISCV Compiler *(riscv-gnu-toolchain)*
* Architecture Modeling Tools + Dataflow Graph Compiler *(ss-scheduler)*
* Example Workloads *(ss-workloads)*

Note on building:
------
 - Follow the provided instructions to build the stream specialized software stack locally
 - There may be some dependences which are not listed, install as necessary for now.

___

Pre-requisites:
---------------

This code is dependent on many libraries.

If you are on Ubuntu, try this:
````
sudo apt-get install autoconf automake autotools-dev curl libmpc-dev libmpfr-dev \
  libgmp-dev gawk build-essential bison flex texinfo gperf libtool patchutils bc \
  qt4-dev-tools libqt4-dev python-dev scons libboost-regex-dev \
  libboost-serialization-dev libgoogle-perftools-dev
````

___

Quick Build Instructions:
------------------------
1. Clone this repo in from git
````
git clone --recursive https://github.com/PolyArch/stream-specialization-stack
````

2. Source the setup.sh script:
````
source setup.sh
````

3. Build the software:
````
make build-all
````
NOTE: DO NOT use `-j`, which may cause dependence problem!

___

Basic Makefile Commands
-----------------------

* A Full Rebuild wipe the install directory and install everything over again.  If folder is in a bad state, try this:
````
make full-rebuild
````

* Incremental rebuild should mostly only do what is necessary (still slower than building one library):
````
make build-all
````

* You can make an individual library by doing this (eg. ss-scheduler).
````
make ss-scheduler
````


Notes on Environment Setup
----------------------

* The setup.sh initialies some environment variables
  * `SS_STACK:` This is the location of this repository
  * `SS_TOOLS:` This is the location of any installed files.

Citation
----------------------

If you use this repository in your work, please consider citing the following:

Stream-Dataflow Acceleration, ISCA 2019, Tony Nowatzki, Vinay Gangadhar, Newsha Ardalani, Karthikeyan Sankaralingam	
https://doi.org/10.1145/3079856.3080255

