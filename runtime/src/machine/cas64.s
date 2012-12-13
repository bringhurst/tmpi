	.file	"cas64.s"
	.version	"01.01"
gcc2_compiled.:
.text
	.align 4
.globl cas64
	.type	 cas64,@function
cas64:
	pushl %ebp
	movl %esp,%ebp
	pushl %esi
	pushl %ebx
	movl 8(%ebp),%esi
	movl 20(%ebp),%ebx
	movl 24(%ebp),%ecx
	movl 12(%ebp),%eax
	movl 16(%ebp),%edx
#APP
	lock ; cmpxchg8b (%esi)
	sete %cl
#NO_APP
	movsbl %cl,%ecx
	movl %ecx,%eax
	leal -8(%ebp),%esp
	popl %ebx
	popl %esi
	leave
	ret
.Lfe2:
	.size	 cas64,.Lfe2-cas64
	.align 4
.globl casd
	.type	 casd,@function
casd:
	pushl %ebp
	movl %esp,%ebp
	pushl %esi
	pushl %ebx
	movl 8(%ebp),%esi
	movl 20(%ebp),%ebx
	movl 24(%ebp),%ecx
	movl 12(%ebp),%eax
	movl 16(%ebp),%edx
#APP
	lock ; cmpxchg8b (%esi)
	sete %cl
#NO_APP
	movsbl %cl,%ecx
	movl %ecx,%eax
	leal -8(%ebp),%esp
	popl %ebx
	popl %esi
	leave
	ret
.Lfe3:
	.size	 casd,.Lfe3-casd
