SHELL := /bin/sh

ENV_H := ../riscv-tests/env/encoding.h
RISCV_BIN := ../riscv-gnu-toolchain/riscv-binutils/

ALL_OPCODES := opcodes-pseudo opcodes opcodes-rvc opcodes-rvc-pseudo opcodes-custom opcodes-ss

SB_OPCODES := opcodes-pseudo opcodes opcodes-rvc opcodes-rvc-pseudo opcodes-custom opcodes-ss

install: inst.chisel instr-table.tex priv-instr-table.tex

sb-install: inst.chisel instr-table.tex priv-instr-table.tex


SS_EXTENSION = $(RISCV_BIN)/include/opcode/ss_opcodes.h \
	       $(RISCV_BIN)/include/opcode/ss_encoding.h \
	       $(RISCV_BIN)/opcodes/ss_textformat.h

$(RISCV_BIN)/include/opcode/ss_opcodes.h: opcodes-ss parse-opcodes
	echo "#ifdef DECLARE_INSN" > $@
	cat opcodes-ss | ./parse-opcodes -c | \
	cpp -P -D DECLARE_INSN=DECLARE_INSN >> $@
	echo "#endif //DECLARE_INSN" >> $@

$(RISCV_BIN)/include/opcode/ss_encoding.h: opcodes-ss parse-opcodes
	echo "#ifndef ENCODINGS_SB" > $@
	echo "#define ENCODINGS_SB" >> $@
	cat opcodes-ss | ./parse-opcodes -c | \
	grep "#define M" >> $@
	echo "#endif //ENCODINGS_SB" >> $@

$(RISCV_BIN)/opcodes/ss_textformat.h: opcodes-ss dump-textformat
	./dump-textformat < textformat-ss > $@

install-ss: $(SS_EXTENSION)

clean-ss:
	rm -f $(SS_EXTENSION)

$(GAS_H) $(XCC_H): $(ALL_OPCODES) parse-opcodes
	cat $(SB_OPCODES) | ./parse-opcodes -c > $@

inst.chisel: $(ALL_OPCODES) parse-opcodes
	cat opcodes opcodes-custom opcodes-pseudo opcodes-ss | ./parse-opcodes -chisel > $@

instr-table.tex: $(ALL_OPCODES) parse-opcodes
	cat opcodes opcodes-pseudo | ./parse-opcodes -tex > $@

priv-instr-table.tex: $(ALL_OPCODES) parse-opcodes
	cat opcodes opcodes-pseudo | ./parse-opcodes -privtex > $@

.PHONY : install
