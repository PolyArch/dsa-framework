PHONY: default
default: build-all

include msg.mk

RVTOOLS = riscv-gnu-toolchain riscv-opcodes

SS_SCHED = ss-scheduler
GEM5     = gem5

MODULES = $(RVTOOLS) $(SS_SCHED) $(GEM5) $(SS_LLVM)
CLEAN_MODULES = $(addprefix clean-,$(MODULES))

.PHONY: $(MODULES) $(CLEAN_MODULES) $(INIT_MODULES)

.PHONY: build-all
build-all: $(MODULES)

.PHONY: clean-all
clean-all: $(CLEAN_MODULES)

$(GEM5): ss-scheduler
	cd $@; scons build/RISCV/gem5.opt -j7

.PHONY: $(SS_SCHED)
$(SS_SCHED):
	make -C $@ install

.PHONY: clean-$(SS_SCHED)
clean-$(SS_SCHED):
	$(MAKE) -C $(patsubst clean-%,%,$@) clean

.PHONY: clean-ss
clean-ss: clean-$(SS_SCHED)

# riscv-pk uses a special set of configure flags

riscv-gnu-toolchain: riscv-opcodes
	mkdir -p $@/build
	cd $@ && autoreconf -fiv && cd build && ../configure --prefix=$(SS_TOOLS)/ --enable-multilib
	$(MAKE) -C $@/build -j9

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
	$(MAKE) clean-ss-scheduler
	$(MAKE) ss-scheduler gem5

.PHONY: build-ss
build-ss: 
	$(MAKE) ss-scheduler gem5

.PHONY: update
update:
	git submodule foreach 'git checkout polyarch; git pull origin polyarch; git submodule update --init'
