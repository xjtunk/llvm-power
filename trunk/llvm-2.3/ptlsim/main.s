	.file	"main.c"
	.data
	.align 4
	.type	running_under_ptlsim, @object
	.size	running_under_ptlsim, 4
running_under_ptlsim:
	.long	-1
	.section	.rodata
.LC0:
	.string	"Done!\n"
	.text
.globl main
	.type	main, @function
main:
	pushl	%ebp
	movl	%esp, %ebp
	subl	$8, %esp
	andl	$-16, %esp
	movl	$0, %eax
	addl	$15, %eax
	addl	$15, %eax
	shrl	$4, %eax
	sall	$4, %eax
	subl	%eax, %esp
	subl	$12, %esp
	pushl	$.LC0
	call	printf
	addl	$16, %esp
	movl	$0, %eax
	leave
	ret
	.size	main, .-main
	.section	.rodata
	.align 4
.LC2:
	.long	1084227584
	.text
.globl break_fn
	.type	break_fn, @function
break_fn:
	pushl	%ebp
	movl	%esp, %ebp
	subl	$4, %esp
#APP
	.byte 0x0f; .byte 0x38; .long 43
#NO_APP
	movl	$0x42040000, %eax
	movl	%eax, -4(%ebp)
	flds	-4(%ebp)
	flds	.LC2
	fmulp	%st, %st(1)
	fstps	-4(%ebp)
#APP
	.byte 0x0f; .byte 0x38; .long 22
#NO_APP
	leave
	ret
	.size	break_fn, .-break_fn
	.section	.note.GNU-stack,"",@progbits
	.ident	"GCC: (GNU) 3.4.6 20060404 (Red Hat 3.4.6-10)"
