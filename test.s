	.file	"test.c"
	.globl	_dlopen_param1_s
	.section	.rodata
.LC0:
	.string	"test.so"
	.data
	.align 8
	.type	_dlopen_param1_s, @object
	.size	_dlopen_param1_s, 8
_dlopen_param1_s:
	.quad	.LC0
	.globl	countf
	.bss
	.align 4
	.type	countf, @object
	.size	countf, 4
countf:
	.zero	4
	.section	.rodata
.LC1:
	.string	"hello world"
	.text
	.globl	main
	.type	main, @function
main:
.LFB0:
	.cfi_startproc
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset 6, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register 6
	subq	$16, %rsp
	movq	_dlopen_param1_s(%rip), %rax
	movl	$2, %esi
	movq	%rax, %rdi
	call	dlopen
	movq	%rax, -8(%rbp)
	movl	$.LC1, %edi
	call	puts
	movl	$1, %eax
	leave
	.cfi_def_cfa 7, 8
	ret
	.cfi_endproc
.LFE0:
	.size	main, .-main
	.ident	"GCC: (Ubuntu 5.4.0-6ubuntu1~16.04.4) 5.4.0 20160609"
	.section	.note.GNU-stack,"",@progbits
