#source: ifunc-2-local-i386.s
#ld: -z now -m elf_i386 -shared --hash-style=sysv
#as: --32
#objdump: -dw
#target: x86_64-*-* i?86-*-*
#notarget: x86_64-*-nacl* i?86-*-nacl*

.*: +file format .*


Disassembly of section .plt:

0+140 <.plt>:
 +[a-f0-9]+:	ff b3 04 00 00 00    	pushl  0x4\(%ebx\)
 +[a-f0-9]+:	ff a3 08 00 00 00    	jmp    \*0x8\(%ebx\)
 +[a-f0-9]+:	00 00                	add    %al,\(%eax\)
	...

0+150 <\*ABS\*@plt>:
 +[a-f0-9]+:	ff a3 0c 00 00 00    	jmp    \*0xc\(%ebx\)
 +[a-f0-9]+:	68 00 00 00 00       	push   \$0x0
 +[a-f0-9]+:	e9 e0 ff ff ff       	jmp    140 <.plt>

Disassembly of section .text:

0+160 <__GI_foo>:
 +[a-f0-9]+:	c3                   	ret    

0+161 <bar>:
 +[a-f0-9]+:	e8 00 00 00 00       	call   166 <bar\+0x5>
 +[a-f0-9]+:	5b                   	pop    %ebx
 +[a-f0-9]+:	81 c3 9e 10 00 00    	add    \$0x109e,%ebx
 +[a-f0-9]+:	e8 de ff ff ff       	call   150 <\*ABS\*@plt>
 +[a-f0-9]+:	8d 83 4c ef ff ff    	lea    -0x10b4\(%ebx\),%eax
 +[a-f0-9]+:	c3                   	ret    
#pass
