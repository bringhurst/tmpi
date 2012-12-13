	.file	"scon.c"
	.version	"01.01"
gcc2_compiled.:
.globl x
.data
	.align 4
	.type	 x,@object
	.size	 x,4
x:
	.long 0
.globl y
	.align 4
	.type	 y,@object
	.size	 y,4
y:
	.long 0
.globl lx
	.align 4
	.type	 lx,@object
	.size	 lx,4
lx:
	.long 0
.globl ly
	.align 4
	.type	 ly,@object
	.size	 ly,4
ly:
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
.globl n
	.align 4
	.type	 n,@object
	.size	 n,4
n:
	.long 100000
.text
	.align 4
.globl main
	.type	 main,@function
main:
	pushl %ebp
	movl %esp,%ebp
	subl $8,%esp
	pushl %esi
	pushl %ebx
	cmpl $1,8(%ebp)
	jle .L32
	pushl $0
	pushl $10
	pushl $0
	movl 12(%ebp),%eax
	pushl 4(%eax)
	call __strtol_internal
	addl $16,%esp
	movl %eax,n
.L32:
	pushl $0
	pushl $sconx
	pushl $0
	leal -8(%ebp),%eax
	pushl %eax
	call pthread_create
	pushl $0
	pushl $scony
	pushl $0
	leal -4(%ebp),%eax
	pushl %eax
	call pthread_create
	xorl %ebx,%ebx
	addl $32,%esp
	leal -8(%ebp),%esi
	.p2align 4,,7
.L38:
	pushl $0
	pushl -8(%ebp)
	call pthread_join
	pushl $0
	pushl 4(%esi)
	call pthread_join
	addl $16,%esp
	incl %ebx
	cmpl $1,%ebx
	jle .L38
	leal -16(%ebp),%esp
	popl %ebx
	popl %esi
	leave
	ret
.Lfe1:
	.size	 main,.Lfe1-main
.section	.rodata
.LC0:
	.string	"start\n"
	.align 32
.LC1:
	.string	"Sequential consistency violated! lx=%d, ly=%d, x=%d, y=%d\n"
.text
	.align 4
.globl sconx
	.type	 sconx,@function
sconx:
	pushl %ebp
	movl %esp,%ebp
	pushl %esi
	pushl %ebx
	pushl $.LC0
	pushl stderr
	call fprintf
	pushl stderr
	call fflush
	movl $1,%ebx
	addl $12,%esp
	cmpl n,%ebx
	jg .L40
	.p2align 4,,7
.L44:
	pushl $1
	pushl $b1
	call anf32
	addl $8,%esp
	.p2align 4,,7
.L45:
	movl b1,%eax
	cmpl $2,%eax
	jne .L45
	pushl $1
	pushl $b2
	call anf32
	addl $8,%esp
	.p2align 4,,7
.L49:
	movl b2,%eax
	cmpl $2,%eax
	jne .L49
#APP
	lock ; movl %ebx, x
	movl y, %esi
	movl %esi, ly
	
#NO_APP
	pushl $1
	pushl $b1
	call snf32
	addl $8,%esp
	.p2align 4,,7
.L53:
	movl b1,%eax
	testl %eax,%eax
	jne .L53
	pushl $1
	pushl $b2
	call snf32
	addl $8,%esp
	.p2align 4,,7
.L57:
	movl b2,%eax
	testl %eax,%eax
	jne .L57
	movl ly,%eax
	cmpl %ebx,%eax
	je .L43
	movl lx,%eax
	cmpl %ebx,%eax
	je .L43
	movl y,%eax
	pushl %eax
	movl x,%eax
	pushl %eax
	movl ly,%eax
	pushl %eax
	movl lx,%eax
	pushl %eax
	pushl $.LC1
	pushl stderr
	call fprintf
	xorl %eax,%eax
	jmp .L40
	.p2align 4,,7
.L43:
	incl %ebx
	cmpl n,%ebx
	jle .L44
.L40:
	leal -8(%ebp),%esp
	popl %ebx
	popl %esi
	leave
	ret
.Lfe2:
	.size	 sconx,.Lfe2-sconx
	.align 4
.globl scony
	.type	 scony,@function
scony:
	pushl %ebp
	movl %esp,%ebp
	pushl %ebx
	pushl $.LC0
	pushl stderr
	call fprintf
	pushl stderr
	call fflush
	movl $1,%ebx
	addl $12,%esp
	cmpl n,%ebx
	jg .L63
	.p2align 4,,7
.L67:
	pushl $1
	pushl $b1
	call anf32
	addl $8,%esp
	.p2align 4,,7
.L68:
	movl b1,%eax
	cmpl $2,%eax
	jne .L68
	pushl $1
	pushl $b2
	call anf32
	addl $8,%esp
	.p2align 4,,7
.L72:
	movl b2,%eax
	cmpl $2,%eax
	jne .L72
	movl %ebx,y
	movl x,%eax
	movl %eax,lx
	pushl $1
	pushl $b1
	call snf32
	addl $8,%esp
	.p2align 4,,7
.L76:
	movl b1,%eax
	testl %eax,%eax
	jne .L76
	pushl $1
	pushl $b2
	call snf32
	addl $8,%esp
	.p2align 4,,7
.L80:
	movl b2,%eax
	testl %eax,%eax
	jne .L80
	movl ly,%eax
	cmpl %ebx,%eax
	je .L66
	movl lx,%eax
	cmpl %ebx,%eax
	je .L66
	movl y,%eax
	pushl %eax
	movl x,%eax
	pushl %eax
	movl ly,%eax
	pushl %eax
	movl lx,%eax
	pushl %eax
	pushl $.LC1
	pushl stderr
	call fprintf
	xorl %eax,%eax
	jmp .L63
	.p2align 4,,7
.L66:
	incl %ebx
	cmpl n,%ebx
	jle .L67
.L63:
	movl -4(%ebp),%ebx
	leave
	ret
.Lfe3:
	.size	 scony,.Lfe3-scony
	.ident	"GCC: (GNU) egcs-2.91.66 19990314/Linux (egcs-1.1.2 release)"
