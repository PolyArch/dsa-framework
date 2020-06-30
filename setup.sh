#!/bin/bash

DIR="$( cd "$( dirname "$0" )" >/dev/null 2>&1 && pwd )"

echo "set SS="$DIR
echo "set SS_STACK="$DIR
echo "set SS_TOOLS="$DIR/ss-tools
echo add "$"SS_TOOLS/bin to "$"PATH
echo add "$"SS_TOOLS/lib to "$"LD_LIBRARY_PATH
echo add gem5/build/RISCV to PATH

export SS=$DIR
export SS_STACK=$SS
export SS_TOOLS=$SS_STACK/ss-tools
export RISCV=$SS_TOOLS
export PATH=$SS_TOOLS/bin:$PATH
export PATH=$SS_STACK/dsa-gem5/build/RISCV:$PATH
export PATH=$SS/scripts:$PATH
export LD_LIBRARY_PATH=$SS_TOOLS/lib:$LD_LIBRARY_PATH

