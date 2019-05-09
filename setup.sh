#!/bin/sh

export SS=`realpath $(dirname "$0")`
export SS_STACK=$SS
export SS_TOOLS=$SS_STACK/ss-tools
export RISCV=$SS_TOOLS
export PATH=$SS_TOOLS/bin:$PATH
export PATH=$SS_STACK/gem5/build/RISCV:$PATH
export LD_LIBRARY_PATH=$SS_TOOLS/lib:$LD_LIBRARY_PATH
