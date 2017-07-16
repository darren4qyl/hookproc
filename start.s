.global _dlopen_addr_s
.global _dlopen_param1_s
.global _dlopen_param2_s

.global _dlsym_addr_s
.global _dlsym_param2_s

.global _dlclose_addr_s

.global _inject_start_s
.global _inject_end_s

.global _inject_function_param_s

.global _saved_cpsr_s
.global _saved_r0_pc_s

.data
_code_start_s:
/*fds*/
.LC0:
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
	movl	$.LC0, %edi
	call	puts
	movl	$1, %eax
	popq	%rbp
	.cfi_def_cfa 7, 8
	ret
	.cfi_endproc
.LFE0:
	.size	main, .-main
	.ident	"GCC: (Ubuntu 5.4.0-6ubuntu1~16.04.4) 5.4.0 20160609"
	.section	.note.GNU-stack,"",@progbits
