#source: bnd-branch-1.s
#as: --64
#ld: -z now -shared -melf_x86_64 -z bndplt --hash-style=sysv
#objdump: -dw

.*: +file format .*


Disassembly of section .plt:

0+290 <.plt>:
 +[a-f0-9]+:	ff 35 a2 01 20 00    	pushq  0x2001a2\(%rip\)        # 200438 <_GLOBAL_OFFSET_TABLE_\+0x8>
 +[a-f0-9]+:	f2 ff 25 a3 01 20 00 	bnd jmpq \*0x2001a3\(%rip\)        # 200440 <_GLOBAL_OFFSET_TABLE_\+0x10>
 +[a-f0-9]+:	0f 1f 00             	nopl   \(%rax\)
 +[a-f0-9]+:	68 00 00 00 00       	pushq  \$0x0
 +[a-f0-9]+:	f2 e9 e5 ff ff ff    	bnd jmpq 290 <.plt>
 +[a-f0-9]+:	0f 1f 44 00 00       	nopl   0x0\(%rax,%rax,1\)
 +[a-f0-9]+:	68 01 00 00 00       	pushq  \$0x1
 +[a-f0-9]+:	f2 e9 d5 ff ff ff    	bnd jmpq 290 <.plt>
 +[a-f0-9]+:	0f 1f 44 00 00       	nopl   0x0\(%rax,%rax,1\)
 +[a-f0-9]+:	68 02 00 00 00       	pushq  \$0x2
 +[a-f0-9]+:	f2 e9 c5 ff ff ff    	bnd jmpq 290 <.plt>
 +[a-f0-9]+:	0f 1f 44 00 00       	nopl   0x0\(%rax,%rax,1\)
 +[a-f0-9]+:	68 03 00 00 00       	pushq  \$0x3
 +[a-f0-9]+:	f2 e9 b5 ff ff ff    	bnd jmpq 290 <.plt>
 +[a-f0-9]+:	0f 1f 44 00 00       	nopl   0x0\(%rax,%rax,1\)

Disassembly of section .plt.sec:

0+2e0 <foo2@plt>:
 +[a-f0-9]+:	f2 ff 25 61 01 20 00 	bnd jmpq \*0x200161\(%rip\)        # 200448 <foo2>
 +[a-f0-9]+:	90                   	nop

0+2e8 <foo3@plt>:
 +[a-f0-9]+:	f2 ff 25 61 01 20 00 	bnd jmpq \*0x200161\(%rip\)        # 200450 <foo3>
 +[a-f0-9]+:	90                   	nop

0+2f0 <foo1@plt>:
 +[a-f0-9]+:	f2 ff 25 61 01 20 00 	bnd jmpq \*0x200161\(%rip\)        # 200458 <foo1>
 +[a-f0-9]+:	90                   	nop

0+2f8 <foo4@plt>:
 +[a-f0-9]+:	f2 ff 25 61 01 20 00 	bnd jmpq \*0x200161\(%rip\)        # 200460 <foo4>
 +[a-f0-9]+:	90                   	nop

Disassembly of section .text:

0+300 <_start>:
 +[a-f0-9]+:	f2 e9 ea ff ff ff    	bnd jmpq 2f0 <foo1@plt>
 +[a-f0-9]+:	e8 d5 ff ff ff       	callq  2e0 <foo2@plt>
 +[a-f0-9]+:	e9 d8 ff ff ff       	jmpq   2e8 <foo3@plt>
 +[a-f0-9]+:	e8 e3 ff ff ff       	callq  2f8 <foo4@plt>
 +[a-f0-9]+:	f2 e8 cd ff ff ff    	bnd callq 2e8 <foo3@plt>
 +[a-f0-9]+:	e9 d8 ff ff ff       	jmpq   2f8 <foo4@plt>
#pass
