	.file	"ttt.c"
	.version	"01.01"
gcc2_compiled.:
.text
	.align 4
.globl main
	.type	 main,@function
main:
	pushl %ebp
	movl %esp,%ebp
	subl $4,%esp
	addl $4,-4(%ebp)
.L1:
	leave
	ret
.Lfe1:
	.size	 main,.Lfe1-main
	.ident	"GCC: (GNU) egcs-2.91.66 19990314/Linux (egcs-1.1.2 release)"
