	.file	"aadd.c"
	.version	"01.01"
gcc2_compiled.:
.globl n
.data
	.align 4
	.type	 n,@object
	.size	 n,4
n:
	.long 0
.globl b1
	.align 4
	.type	 b1,@object
	.size	 b1,4
b1:
	.long 0
.globl b2
	.align 4
	.type	 b2,@object
	.size	 b2,4
b2:
	.long 0
.text
	.align 4
.globl worker
	.type	 worker,@function
worker:
	pushl %ebp
	movl %esp,%ebp
	subl $4,%esp
	movl $0,-4(%ebp)
#APP
	lock; addl $1, b1
	.p2align 4,,7
#NO_APP
.L2:
	cmpl $10,b1
	jne .L4
	jmp .L3
	.p2align 4,,7
.L4:
	jmp .L2
	.p2align 4,,7
.L3:
#APP
	lock; addl $1, b2
	.p2align 4,,7
#NO_APP
.L5:
	cmpl $10,b2
	jne .L7
	jmp .L6
	.p2align 4,,7
.L7:
	jmp .L5
	.p2align 4,,7
.L6:
	nop
	.p2align 4,,7
.L8:
	cmpl $999999,-4(%ebp)
	jle .L11
	jmp .L9
	.p2align 4,,7
.L11:
	movl $1,%edx
#APP
	lock; addl %edx, m
#NO_APP
	movl $1,%edx
#APP
	lock; subl %edx, n
#NO_APP
.L10:
	incl -4(%ebp)
	jmp .L8
	.p2align 4,,7
.L9:
#APP
	lock; subl $1, b1
	.p2align 4,,7
#NO_APP
.L12:
	cmpl $0,b1
	jne .L14
	jmp .L13
	.p2align 4,,7
.L14:
	jmp .L12
	.p2align 4,,7
.L13:
#APP
	lock; subl $1, b2
	.p2align 4,,7
#NO_APP
.L15:
	cmpl $0,b2
	jne .L17
	jmp .L16
	.p2align 4,,7
.L17:
	jmp .L15
	.p2align 4,,7
.L16:
	xorl %eax,%eax
	jmp .L1
	.p2align 4,,7
.L1:
	leave
	ret
.Lfe1:
	.size	 worker,.Lfe1-worker
.section	.rodata
.LC0:
	.string	"%d, %d\n"
.LC1:
	.string	"Success.\n"
.LC2:
	.string	"Failed.\n"
.text
	.align 4
.globl main
	.type	 main,@function
main:
	pushl %ebp
	movl %esp,%ebp
	subl $44,%esp
	movl $0,-4(%ebp)
	movl $0,-4(%ebp)
	.p2align 4,,7
.L19:
	cmpl $9,-4(%ebp)
	jle .L22
	jmp .L20
	.p2align 4,,7
.L22:
	pushl $0
	pushl $worker
	pushl $0
	leal -44(%ebp),%eax
	movl -4(%ebp),%edx
	movl %edx,%ecx
	leal 0(,%ecx,4),%edx
	addl %edx,%eax
	pushl %eax
	call pthread_create
	addl $16,%esp
.L21:
	incl -4(%ebp)
	jmp .L19
	.p2align 4,,7
.L20:
	nop
	movl $0,-4(%ebp)
	.p2align 4,,7
.L23:
	cmpl $9,-4(%ebp)
	jle .L26
	jmp .L24
	.p2align 4,,7
.L26:
	pushl $0
	movl -4(%ebp),%eax
	movl %eax,%edx
	leal 0(,%edx,4),%eax
	leal -44(%ebp),%edx
	movl (%eax,%edx),%eax
	pushl %eax
	call pthread_join
	addl $8,%esp
.L25:
	incl -4(%ebp)
	jmp .L23
	.p2align 4,,7
.L24:
	movl n,%eax
	pushl %eax
	movl m,%eax
	pushl %eax
	pushl $.LC0
	call printf
	addl $12,%esp
	cmpl $10000000,m
	jne .L27
	cmpl $-10000000,n
	jne .L27
	pushl $.LC1
	call printf
	addl $4,%esp
	jmp .L28
	.p2align 4,,7
.L27:
	pushl $.LC2
	call printf
	addl $4,%esp
.L28:
	xorl %eax,%eax
	jmp .L18
	.p2align 4,,7
.L18:
	leave
	ret
.Lfe2:
	.size	 main,.Lfe2-main
	.comm	m,4,4
	.ident	"GCC: (GNU) egcs-2.91.66 19990314/Linux (egcs-1.1.2 release)"
