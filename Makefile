PHONY: default
default: build-all

include msg.mk

RVTOOLS = dsa-riscv-gnu-toolchain dsa-riscv-opcodes

SS_SCHED = spatial-scheduler
GEM5     = dsa-gem5

SS_LLVM = ss-llvm

MODULES = $(RVTOOLS) $(SS_SCHED) $(GEM5) $(SS_LLVM)
CLEAN_MODULES = $(addprefix clean-,$(MODULES))

.PHONY: $(MODULES) $(CLEAN_MODULES) $(INIT_MODULES)

.PHONY: build-all
build-all: $(MODULES)

.PHONY: clean-all
clean-all: $(CLEAN_MODULES)

$(GEM5): spatial-scheduler
	cd $@; scons build/RISCV/gem5.opt -j7

$(GEM5)-debug: spatial-scheduler
	cd $(GEM5); scons build/RISCV/gem5.debug -j7

.PHONY: $(SS_SCHED)
$(SS_SCHED):
	mkdir -p $@/build
	cd $@/build && cp ../config.cmake . && cmake ..
	# cmake it twice to include the ssinst.h
	cd $@/build && cp ../config.cmake . && cmake ..
	make -C $@/build install -j

.PHONY: clean-$(SS_SCHED)
clean-$(SS_SCHED):
	$(MAKE) -C $(patsubst clean-%,%,$@) clean

.PHONY: clean-ss
clean-ss: clean-$(SS_SCHED)

# riscv-pk uses a special set of configure flags

dsa-riscv-gnu-toolchain: dsa-riscv-opcodes
	mkdir -p $@/build
	cd $@ && autoreconf -fiv && cd build && ../configure --prefix=$(SS_TOOLS)/ # --enable-multilib
	$(MAKE) linux -C $@/build -j9

ss-llvm:
	cd $@; mkdir -p build; cd build;                                    \
	cmake -G "Unix Makefiles"                                           \
	      -DBUILD_SHARED_LIBS=ON -DLLVM_USE_SPLIT_DWARF=ON              \
              -DCMAKE_INSTALL_PREFIX=$SS_TOOLS -DLLVM_OPTIMIZED_TABLEGEN=ON \
              -DLLVM_BUILD_TESTS=False -DLLVM_TARGETS_TO_BUILD=""           \
              -DLLVM_EXPERIMENTAL_TARGETS_TO_BUILD="RISCV"                  \
              -DLLVM_DEFAULT_TARGET_TRIPLE=riscv64-unknown-elf              \
              -DCMAKE_CROSSCOMPILING=True -DLLVM_ENABLE_RTTI=ON             \
              -DLLVM_ENABLE_PROJECTS="clang" ../llvm
	make -C $@/build install -j9

riscv-opcodes:
	make -C $@ install-ss
	mkdir -p $(SS_TOOLS)/include/ss-intrin
	ln -sfn $(SS)/$@/ss_insts.h $(SS_TOOLS)/include/ss-intrin/ss_insts.h

$(addprefix clean-,$(RVTOOLS)):
	rm -rf $(patsubst clean-%,%,$@)/build

full-rebuild:
	@echo "Wipe \$$SS_TOOLS ($$SS_TOOLS) and rebuild everything?"
	@read -p "[Y/n]: " yn && { [ -z $$yn ] || [ $$yn = Y ] || [ $$yn = y ]; }
	rm -rf "$$SS_TOOLS"
	$(MAKE) clean-all
	$(MAKE) build-all

.PHONY: rebuild-ss
rebuild-ss: 
	$(MAKE) clean-spatial-scheduler
	$(MAKE) spatial-scheduler gem5

.PHONY: build-ss
build-ss: 
	$(MAKE) spatial-scheduler gem5

.PHONY: update
update:
	git submodule foreach 'git checkout polyarch; git pull origin polyarch; git submodule update --init'
