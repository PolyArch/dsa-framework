default: all

include msg.mk

.PHONY: all
all: dsa-scheduler chipyard dsa-llvm-project dsa-gem5
	echo "Please source chipyard/env.sh if this is a first-time build"

.PHONY: clean
clean: clean-gem5 clean-llvm clean-chipyard clean-scheduler
	rm -rf ss-tools

# DSA RISC-V Extension
.PHONY: dsa-ext
dsa-ext:
	make -C dsa-riscv-ext COMPAT=1

# Spatial Scheduler
.PHONY: dsa-scheduler
dsa-scheduler: dsa-ext
	make -C $@ all

clean-scheduler:
	rm -rf dsa-scheduler/build
	rm -rf dsa-scheduler/libtorch dsa-scheduler/libtorch.zip

# ChipYard and RISC-V GNU Toolchain
.PHONY: chipyard
chipyard: dsa-ext
ifneq ($(wildcard $(SS)/chipyard/env-riscv-tools.sh),)
	echo "ChipYard RISC-V ENV Script Found, skip rebuild chipyard toolchain"
else
	echo "ChipYard RISC-V ENV Script Not Found, Building Toolchain ... "
	cd ./chipyard && ./scripts/build-toolchains.sh --ignore-qemu riscv-tools
	echo "Please source chipyard/env.sh"
endif

clean-chipyard:
	rm -rf chipyard/riscv-tools-install chipyard/env.sh chipyard/env-riscv-tools.sh

# LLVM compiler
.PHONY: dsa-llvm-project
dsa-llvm-project: chipyard dsa-scheduler
	cd $@ && mkdir -p build && cd build &&                          \
	cmake -G "Unix Makefiles" -DCMAKE_BUILD_TYPE="Release"          \
        -DBUILD_SHARED_LIBS=ON -DLLVM_USE_SPLIT_DWARF=ON                \
        -DCMAKE_INSTALL_PREFIX=$(SS_TOOLS) -DLLVM_OPTIMIZED_TABLEGEN=ON \
        -DLLVM_BUILD_TESTS=False -DLLVM_TARGETS_TO_BUILD="RISCV"        \
        -DLLVM_DEFAULT_TARGET_TRIPLE="riscv64-unknown-linux-gnu"        \
        -DCMAKE_CROSSCOMPILING=True -DLLVM_ENABLE_RTTI=ON               \
        -DLLVM_ENABLE_PROJECTS="clang" ../llvm
	make -C $@/build install -j$$((`nproc`))

clean-llvm: clean-scheduler
	rm -rf dsa-llvm-project/build

# Gem5 simulator
.PHONY: dsa-gem5
dsa-gem5: chipyard dsa-scheduler
	source chipyard/env.sh && cd $@ && scons build/RISCV/gem5.opt build/RISCV/gem5.debug -j`nproc`

clean-gem5: clean-scheduler
	rm -rf dsa-gem5/build
