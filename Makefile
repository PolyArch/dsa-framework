default: all

include msg.mk

.PHONY: all
all: dsa-gem5 spatial-scheduler riscv-gnu-toolchain dsa-llvm-project

.PHONY: clean
clean: clean-gem5 clean-scheduler clean-gnu clean-llvm
	rm -rf ss-tools

.PHONY: dsa-gem5
dsa-gem5: spatial-scheduler
	cd $@ && scons build/RISCV/gem5.opt build/RISCV/gem5.debug -j`nproc`

clean-gem5:
	cd dsa-gem5 && scons -c build/RISCV/gem5.opt build/RISCV/gem5.debug -j`nproc`

.PHONY: spatial-scheduler
spatial-scheduler:
	mkdir -p $@/build
	cd $@/build && cp ../config.cmake . &&      \
	cmake .. -DCMAKE_INSTALL_PREFIX=$(SS_TOOLS)
	make -C $@/build install -j

clean-scheduler:
	make -C spatial-scheduler/build clean

.PHONY: riscv-gnu-patch
riscv-gnu-patch:
	make -C dsa-riscv-ext

.PHONY: riscv-gnu-toolchain
riscv-gnu-toolchain: riscv-gnu-patch
	cd $@ && ./configure --prefix=$(SS_TOOLS)/ --enable-multilib &&  \
	make -j &&                                                       \
	LD_LIBRARY_PATH="" make linux -j

clean-gnu:
	make -C riscv-gnu-toolchain clean

.PHONY: llvm-project
dsa-llvm-project: riscv-gnu-toolchain
	cd $@ && mkdir -p build && cd build &&                          \
	cmake -G "Unix Makefiles" -DCMAKE_BUILD_TYPE="Release"          \
        -DBUILD_SHARED_LIBS=ON -DLLVM_USE_SPLIT_DWARF=ON                \
        -DCMAKE_INSTALL_PREFIX=$(SS_TOOLS) -DLLVM_OPTIMIZED_TABLEGEN=ON \
        -DLLVM_BUILD_TESTS=False -DLLVM_TARGETS_TO_BUILD=""             \
        -DLLVM_EXPERIMENTAL_TARGETS_TO_BUILD="RISCV"                    \
        -DLLVM_TARGETS_TO_BUILD="RISCV"                                 \
        -DDEFAULT_SYSROOT=$(SS_TOOLS)/riscv64-unknown-elf               \
        -DLLVM_DEFAULT_TARGET_TRIPLE="riscv64-unknown-elf"              \
        -DCMAKE_CROSSCOMPILING=True -DLLVM_ENABLE_RTTI=ON               \
        -DLLVM_ENABLE_PROJECTS="clang" ../llvm
	make -C $@/build install -j$$((`nproc`/2))

clean-llvm:
	make -C dsa-llvm-project/build clean
