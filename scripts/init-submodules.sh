#!/usr/bin/env bash

# Get the current path
PWD=$(pwd)
REPO_DIR="$(dirname "$(readlink -f "${BASH_SOURCE[0]:-${(%):-%x}}")")"/../
cd $REPO_DIR

# Initialize non-chipyard repo
git submodule update --init --recursive docs dsa-riscv-ext dsa-gem5 dsa-llvm-project dsa-scheduler dsa-apps

# Initialize ChipYard repo
git submodule update --init chipyard

# Initialize ChipYard submodule
cd chipyard
git config --global url."https://".insteadOf git://
./scripts/init-submodules-no-riscv-tools.sh --skip-validate

# Initialize ChipYard FPGA repo
#./scripts/init-fpga.sh

# Initialize FireMarshal
#cd software/firemarshal
#./init-submodules.sh
#cd ../..

# Initialize risc-v gnu for DSA ISA extension patch
cd ./toolchains/riscv-tools
git submodule update --init riscv-gnu-toolchain
cd ./riscv-gnu-toolchain
git submodule update --init riscv-binutils

# Return to current path
cd $PWD
