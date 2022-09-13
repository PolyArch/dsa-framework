if [[ -z "${SS}" ]]; then
  echo "Please source setup.sh under ss-stack before initialize submodule"
  exit 1
fi

# Get the current path
PWD=$(pwd)

# Initialize non-chipyard repo
cd $SS
git submodule update --init --recursive dsa-riscv-ext gem5 llvm-project ss-scheduler ss-workloads

# Initialize ChipYard repo
git submodule update --init chipyard

# Initialize ChipYard submodule
cd chipyard
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
