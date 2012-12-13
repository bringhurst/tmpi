	.file	"longlong.c"
	.version	"01.01"
gcc2_compiled.:
.globl a
.data
	.align 4
	.type	 a,@object
	.size	 a,8
a:
	.long 10
	.long 0
.section	.rodata
.LC0:
	.string	"%d\n"
.text
	.align 4
.globl main
	.type	 main,@function
main:
	pushl %ebp
	movl %esp,%ebp
	addl $100,a
	adcl $0,a+4
	movl a,%eax
	movl a+4,%edx
	pushl %edx
	pushl %eax
	pushl $.LC0
	call printf
	addl $12,%esp
	xorl %eax,%eax
	jmp .L1
	.p2align 4,,7
.L1:
	leave
	ret
.Lfe1:
	.size	 main,.Lfe1-main
	.ident	"GCC: (GNU) egcs-2.91.66 19990314/Linux (egcs-1.1.2 release)"
