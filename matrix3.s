	.section	__TEXT,__text,regular,pure_instructions
	.macosx_version_min 10, 10
	.globl	_mat3_from_array
	.align	4, 0x90
_mat3_from_array:                       ## @mat3_from_array
	.cfi_startproc
## BB#0:
	pushq	%rbp
Ltmp0:
	.cfi_def_cfa_offset 16
Ltmp1:
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
Ltmp2:
	.cfi_def_cfa_register %rbp
	movups	(%rsi), %xmm0
	movdqu	16(%rsi), %xmm1
	movups	%xmm0, (%rdi)
	movdqu	%xmm1, 16(%rdi)
	pshufd	$231, %xmm1, %xmm0      ## xmm0 = xmm1[3,1,2,3]
	movd	%xmm0, 32(%rdi)
	movq	%rdi, %rax
	popq	%rbp
	retq
	.cfi_endproc

	.globl	_mat3_to_array
	.align	4, 0x90
_mat3_to_array:                         ## @mat3_to_array
	.cfi_startproc
## BB#0:
	pushq	%rbp
Ltmp3:
	.cfi_def_cfa_offset 16
Ltmp4:
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
Ltmp5:
	.cfi_def_cfa_register %rbp
	cmpl	$9, %esi
	jne	LBB1_2
## BB#1:
	leaq	16(%rbp), %rax
	movl	32(%rax), %ecx
	movl	%ecx, 32(%rdi)
	movq	24(%rax), %rcx
	movq	%rcx, 24(%rdi)
	movq	16(%rax), %rcx
	movq	%rcx, 16(%rdi)
	movq	(%rax), %rcx
	movq	8(%rax), %rax
	movq	%rax, 8(%rdi)
	movq	%rcx, (%rdi)
	popq	%rbp
	retq
LBB1_2:
	leaq	L___func__.mat3_to_array(%rip), %rdi
	leaq	L_.str(%rip), %rsi
	leaq	L_.str1(%rip), %rcx
	movl	$21, %edx
	callq	___assert_rtn
	.cfi_endproc

	.globl	_mat3_ident
	.align	4, 0x90
_mat3_ident:                            ## @mat3_ident
	.cfi_startproc
## BB#0:
	pushq	%rbp
Ltmp6:
	.cfi_def_cfa_offset 16
Ltmp7:
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
Ltmp8:
	.cfi_def_cfa_register %rbp
	subq	$48, %rsp
	movq	___stack_chk_guard@GOTPCREL(%rip), %rax
	movq	(%rax), %rax
	movq	%rax, -8(%rbp)
	movl	$0, -16(%rbp)
	movq	$0, -24(%rbp)
	movl	$0, -32(%rbp)
	movq	$0, -40(%rbp)
	movl	$1065353216, (%rdi)     ## imm = 0x3F800000
	movl	-16(%rbp), %ecx
	movl	%ecx, 12(%rdi)
	movq	-24(%rbp), %rcx
	movq	%rcx, 4(%rdi)
	movl	$1065353216, 16(%rdi)   ## imm = 0x3F800000
	movl	-32(%rbp), %ecx
	movl	%ecx, 28(%rdi)
	movq	-40(%rbp), %rcx
	movq	%rcx, 20(%rdi)
	movl	$1065353216, 32(%rdi)   ## imm = 0x3F800000
	cmpq	-8(%rbp), %rax
	jne	LBB2_2
## BB#1:
	movq	%rdi, %rax
	addq	$48, %rsp
	popq	%rbp
	retq
LBB2_2:
	callq	___stack_chk_fail
	.cfi_endproc

	.globl	_mat3_mult
	.align	4, 0x90
_mat3_mult:                             ## @mat3_mult
	.cfi_startproc
## BB#0:
	pushq	%rbp
Ltmp9:
	.cfi_def_cfa_offset 16
Ltmp10:
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
Ltmp11:
	.cfi_def_cfa_register %rbp
	movss	64(%rbp), %xmm8         ## xmm8 = mem[0],zero,zero,zero
	movss	76(%rbp), %xmm11        ## xmm11 = mem[0],zero,zero,zero
	movss	88(%rbp), %xmm10        ## xmm10 = mem[0],zero,zero,zero
	movss	56(%rbp), %xmm9         ## xmm9 = mem[0],zero,zero,zero
	movss	60(%rbp), %xmm12        ## xmm12 = mem[0],zero,zero,zero
	movss	16(%rbp), %xmm7         ## xmm7 = mem[0],zero,zero,zero
	movss	20(%rbp), %xmm5         ## xmm5 = mem[0],zero,zero,zero
	movaps	%xmm7, %xmm2
	unpcklps	%xmm9, %xmm2    ## xmm2 = xmm2[0],xmm9[0],xmm2[1],xmm9[1]
	unpcklps	%xmm7, %xmm7    ## xmm7 = xmm7[0,0,1,1]
	unpcklps	%xmm2, %xmm7    ## xmm7 = xmm7[0],xmm2[0],xmm7[1],xmm2[1]
	movss	28(%rbp), %xmm14        ## xmm14 = mem[0],zero,zero,zero
	movaps	%xmm12, %xmm2
	unpcklps	%xmm14, %xmm2   ## xmm2 = xmm2[0],xmm14[0],xmm2[1],xmm14[1]
	movaps	%xmm8, %xmm13
	unpcklps	%xmm12, %xmm13  ## xmm13 = xmm13[0],xmm12[0],xmm13[1],xmm12[1]
	unpcklps	%xmm9, %xmm12   ## xmm12 = xmm12[0],xmm9[0],xmm12[1],xmm9[1]
	unpcklps	%xmm8, %xmm9    ## xmm9 = xmm9[0],xmm8[0],xmm9[1],xmm8[1]
	unpcklps	%xmm2, %xmm9    ## xmm9 = xmm9[0],xmm2[0],xmm9[1],xmm2[1]
	mulps	%xmm7, %xmm9
	movss	68(%rbp), %xmm7         ## xmm7 = mem[0],zero,zero,zero
	movaps	%xmm5, %xmm2
	unpcklps	%xmm7, %xmm2    ## xmm2 = xmm2[0],xmm7[0],xmm2[1],xmm7[1]
	unpcklps	%xmm5, %xmm5    ## xmm5 = xmm5[0,0,1,1]
	unpcklps	%xmm2, %xmm5    ## xmm5 = xmm5[0],xmm2[0],xmm5[1],xmm2[1]
	movss	32(%rbp), %xmm2         ## xmm2 = mem[0],zero,zero,zero
	movss	72(%rbp), %xmm0         ## xmm0 = mem[0],zero,zero,zero
	movaps	%xmm0, %xmm1
	unpcklps	%xmm2, %xmm1    ## xmm1 = xmm1[0],xmm2[0],xmm1[1],xmm2[1]
	movaps	%xmm11, %xmm15
	unpcklps	%xmm0, %xmm15   ## xmm15 = xmm15[0],xmm0[0],xmm15[1],xmm0[1]
	unpcklps	%xmm7, %xmm0    ## xmm0 = xmm0[0],xmm7[0],xmm0[1],xmm7[1]
	unpcklps	%xmm11, %xmm7   ## xmm7 = xmm7[0],xmm11[0],xmm7[1],xmm11[1]
	unpcklps	%xmm1, %xmm7    ## xmm7 = xmm7[0],xmm1[0],xmm7[1],xmm1[1]
	mulps	%xmm5, %xmm7
	addps	%xmm9, %xmm7
	movss	80(%rbp), %xmm9         ## xmm9 = mem[0],zero,zero,zero
	movss	24(%rbp), %xmm5         ## xmm5 = mem[0],zero,zero,zero
	movaps	%xmm5, %xmm6
	unpcklps	%xmm9, %xmm6    ## xmm6 = xmm6[0],xmm9[0],xmm6[1],xmm9[1]
	unpcklps	%xmm5, %xmm5    ## xmm5 = xmm5[0,0,1,1]
	unpcklps	%xmm6, %xmm5    ## xmm5 = xmm5[0],xmm6[0],xmm5[1],xmm6[1]
	movss	36(%rbp), %xmm6         ## xmm6 = mem[0],zero,zero,zero
	movss	84(%rbp), %xmm4         ## xmm4 = mem[0],zero,zero,zero
	movaps	%xmm4, %xmm3
	unpcklps	%xmm6, %xmm3    ## xmm3 = xmm3[0],xmm6[0],xmm3[1],xmm6[1]
	movaps	%xmm10, %xmm1
	unpcklps	%xmm4, %xmm1    ## xmm1 = xmm1[0],xmm4[0],xmm1[1],xmm4[1]
	unpcklps	%xmm9, %xmm4    ## xmm4 = xmm4[0],xmm9[0],xmm4[1],xmm9[1]
	unpcklps	%xmm10, %xmm9   ## xmm9 = xmm9[0],xmm10[0],xmm9[1],xmm10[1]
	unpcklps	%xmm3, %xmm9    ## xmm9 = xmm9[0],xmm3[0],xmm9[1],xmm3[1]
	mulps	%xmm5, %xmm9
	addps	%xmm7, %xmm9
	unpcklps	%xmm13, %xmm12  ## xmm12 = xmm12[0],xmm13[0],xmm12[1],xmm13[1]
	movss	40(%rbp), %xmm3         ## xmm3 = mem[0],zero,zero,zero
	unpcklps	%xmm3, %xmm14   ## xmm14 = xmm14[0],xmm3[0],xmm14[1],xmm3[1]
	unpcklps	%xmm14, %xmm14  ## xmm14 = xmm14[0,0,1,1]
	mulps	%xmm12, %xmm14
	unpcklps	%xmm15, %xmm0   ## xmm0 = xmm0[0],xmm15[0],xmm0[1],xmm15[1]
	movss	44(%rbp), %xmm5         ## xmm5 = mem[0],zero,zero,zero
	unpcklps	%xmm5, %xmm2    ## xmm2 = xmm2[0],xmm5[0],xmm2[1],xmm5[1]
	unpcklps	%xmm2, %xmm2    ## xmm2 = xmm2[0,0,1,1]
	mulps	%xmm0, %xmm2
	addps	%xmm14, %xmm2
	unpcklps	%xmm1, %xmm4    ## xmm4 = xmm4[0],xmm1[0],xmm4[1],xmm1[1]
	movss	48(%rbp), %xmm0         ## xmm0 = mem[0],zero,zero,zero
	unpcklps	%xmm0, %xmm6    ## xmm6 = xmm6[0],xmm0[0],xmm6[1],xmm0[1]
	unpcklps	%xmm6, %xmm6    ## xmm6 = xmm6[0,0,1,1]
	mulps	%xmm4, %xmm6
	addps	%xmm2, %xmm6
	mulss	%xmm3, %xmm8
	mulss	%xmm5, %xmm11
	addss	%xmm8, %xmm11
	mulss	%xmm0, %xmm10
	addss	%xmm11, %xmm10
	movups	%xmm9, (%rdi)
	movups	%xmm6, 16(%rdi)
	movss	%xmm10, 32(%rdi)
	movq	%rdi, %rax
	popq	%rbp
	retq
	.cfi_endproc

	.globl	_mat3_multvec
	.align	4, 0x90
_mat3_multvec:                          ## @mat3_multvec
	.cfi_startproc
## BB#0:
	pushq	%rbp
Ltmp12:
	.cfi_def_cfa_offset 16
Ltmp13:
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
Ltmp14:
	.cfi_def_cfa_register %rbp
	movshdup	%xmm0, %xmm3    ## xmm3 = xmm0[1,1,3,3]
	movss	28(%rbp), %xmm2         ## xmm2 = mem[0],zero,zero,zero
	movss	16(%rbp), %xmm4         ## xmm4 = mem[0],zero,zero,zero
	movss	20(%rbp), %xmm5         ## xmm5 = mem[0],zero,zero,zero
	unpcklps	%xmm2, %xmm4    ## xmm4 = xmm4[0],xmm2[0],xmm4[1],xmm2[1]
	movsldup	%xmm0, %xmm2    ## xmm2 = xmm0[0,0,2,2]
	mulps	%xmm4, %xmm2
	movss	32(%rbp), %xmm4         ## xmm4 = mem[0],zero,zero,zero
	unpcklps	%xmm4, %xmm5    ## xmm5 = xmm5[0],xmm4[0],xmm5[1],xmm4[1]
	mulps	%xmm3, %xmm5
	addps	%xmm2, %xmm5
	movsldup	%xmm1, %xmm4    ## xmm4 = xmm1[0,0,2,2]
	movss	36(%rbp), %xmm6         ## xmm6 = mem[0],zero,zero,zero
	movss	24(%rbp), %xmm2         ## xmm2 = mem[0],zero,zero,zero
	unpcklps	%xmm6, %xmm2    ## xmm2 = xmm2[0],xmm6[0],xmm2[1],xmm6[1]
	mulps	%xmm4, %xmm2
	addps	%xmm5, %xmm2
	movss	40(%rbp), %xmm4         ## xmm4 = mem[0],zero,zero,zero
	mulss	%xmm0, %xmm4
	movss	44(%rbp), %xmm0         ## xmm0 = mem[0],zero,zero,zero
	mulss	%xmm3, %xmm0
	addss	%xmm4, %xmm0
	mulss	48(%rbp), %xmm1
	addss	%xmm0, %xmm1
	movaps	%xmm2, %xmm0
	popq	%rbp
	retq
	.cfi_endproc

	.section	__TEXT,__literal4,4byte_literals
	.align	2
LCPI5_0:
	.long	1065353216              ## float 1
	.section	__TEXT,__text,regular,pure_instructions
	.globl	_mat3_rot
	.align	4, 0x90
_mat3_rot:                              ## @mat3_rot
	.cfi_startproc
## BB#0:
	pushq	%rbp
Ltmp15:
	.cfi_def_cfa_offset 16
Ltmp16:
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
Ltmp17:
	.cfi_def_cfa_register %rbp
	pushq	%rbx
	subq	$40, %rsp
Ltmp18:
	.cfi_offset %rbx, -24
	movaps	%xmm2, -32(%rbp)        ## 16-byte Spill
	movss	%xmm1, -36(%rbp)        ## 4-byte Spill
	movss	%xmm0, -40(%rbp)        ## 4-byte Spill
	movq	%rdi, %rbx
	xorps	%xmm0, %xmm0
	cvtss2sd	%xmm3, %xmm0
	callq	___sincos_stret
	cvtsd2ss	%xmm0, %xmm10
	cvtsd2ss	%xmm1, %xmm13
	movss	LCPI5_0(%rip), %xmm1    ## xmm1 = mem[0],zero,zero,zero
	subss	%xmm13, %xmm1
	movss	-40(%rbp), %xmm7        ## 4-byte Reload
                                        ## xmm7 = mem[0],zero,zero,zero
	movaps	%xmm7, %xmm4
	mulss	%xmm4, %xmm4
	mulss	%xmm1, %xmm4
	addss	%xmm13, %xmm4
	movaps	%xmm7, %xmm3
	movss	-36(%rbp), %xmm6        ## 4-byte Reload
                                        ## xmm6 = mem[0],zero,zero,zero
	mulss	%xmm6, %xmm3
	mulss	%xmm1, %xmm3
	movaps	%xmm10, %xmm5
	movaps	-32(%rbp), %xmm0        ## 16-byte Reload
	mulss	%xmm0, %xmm5
	movaps	%xmm3, %xmm8
	subss	%xmm5, %xmm8
	movaps	%xmm7, %xmm2
	movaps	%xmm7, %xmm11
	mulss	%xmm0, %xmm2
	movaps	%xmm0, %xmm12
	mulss	%xmm1, %xmm2
	movaps	%xmm10, %xmm0
	mulss	%xmm6, %xmm0
	movaps	%xmm0, %xmm9
	addss	%xmm2, %xmm9
	addss	%xmm3, %xmm5
	movaps	%xmm6, %xmm3
	movaps	%xmm3, %xmm7
	mulss	%xmm7, %xmm7
	mulss	%xmm1, %xmm7
	addss	%xmm13, %xmm7
	mulss	%xmm12, %xmm3
	mulss	%xmm1, %xmm3
	mulss	%xmm11, %xmm10
	movaps	%xmm3, %xmm11
	subss	%xmm10, %xmm11
	subss	%xmm0, %xmm2
	addss	%xmm3, %xmm10
	movaps	%xmm12, %xmm0
	mulss	%xmm0, %xmm0
	mulss	%xmm1, %xmm0
	addss	%xmm13, %xmm0
	movaps	%xmm0, %xmm14
	movaps	%xmm9, %xmm12
	unpcklps	%xmm8, %xmm12   ## xmm12 = xmm12[0],xmm8[0],xmm12[1],xmm8[1]
	unpcklps	%xmm4, %xmm8    ## xmm8 = xmm8[0],xmm4[0],xmm8[1],xmm4[1]
	unpcklps	%xmm9, %xmm4    ## xmm4 = xmm4[0],xmm9[0],xmm4[1],xmm9[1]
	unpcklps	%xmm8, %xmm4    ## xmm4 = xmm4[0],xmm8[0],xmm4[1],xmm8[1]
	movss	28(%rbp), %xmm1         ## xmm1 = mem[0],zero,zero,zero
	movss	16(%rbp), %xmm6         ## xmm6 = mem[0],zero,zero,zero
	movss	20(%rbp), %xmm3         ## xmm3 = mem[0],zero,zero,zero
	movaps	%xmm6, %xmm0
	unpcklps	%xmm1, %xmm0    ## xmm0 = xmm0[0],xmm1[0],xmm0[1],xmm1[1]
	unpcklps	%xmm6, %xmm6    ## xmm6 = xmm6[0,0,1,1]
	unpcklps	%xmm0, %xmm6    ## xmm6 = xmm6[0],xmm0[0],xmm6[1],xmm0[1]
	mulps	%xmm4, %xmm6
	movaps	%xmm11, %xmm13
	unpcklps	%xmm7, %xmm13   ## xmm13 = xmm13[0],xmm7[0],xmm13[1],xmm7[1]
	unpcklps	%xmm5, %xmm7    ## xmm7 = xmm7[0],xmm5[0],xmm7[1],xmm5[1]
	unpcklps	%xmm11, %xmm5   ## xmm5 = xmm5[0],xmm11[0],xmm5[1],xmm11[1]
	unpcklps	%xmm7, %xmm5    ## xmm5 = xmm5[0],xmm7[0],xmm5[1],xmm7[1]
	movss	32(%rbp), %xmm4         ## xmm4 = mem[0],zero,zero,zero
	movaps	%xmm3, %xmm0
	unpcklps	%xmm4, %xmm0    ## xmm0 = xmm0[0],xmm4[0],xmm0[1],xmm4[1]
	unpcklps	%xmm3, %xmm3    ## xmm3 = xmm3[0,0,1,1]
	unpcklps	%xmm0, %xmm3    ## xmm3 = xmm3[0],xmm0[0],xmm3[1],xmm0[1]
	mulps	%xmm5, %xmm3
	addps	%xmm6, %xmm3
	movss	36(%rbp), %xmm0         ## xmm0 = mem[0],zero,zero,zero
	movss	24(%rbp), %xmm5         ## xmm5 = mem[0],zero,zero,zero
	movaps	%xmm5, %xmm6
	unpcklps	%xmm0, %xmm6    ## xmm6 = xmm6[0],xmm0[0],xmm6[1],xmm0[1]
	unpcklps	%xmm5, %xmm5    ## xmm5 = xmm5[0,0,1,1]
	unpcklps	%xmm6, %xmm5    ## xmm5 = xmm5[0],xmm6[0],xmm5[1],xmm6[1]
	movaps	%xmm14, %xmm6
	unpcklps	%xmm10, %xmm6   ## xmm6 = xmm6[0],xmm10[0],xmm6[1],xmm10[1]
	unpcklps	%xmm2, %xmm10   ## xmm10 = xmm10[0],xmm2[0],xmm10[1],xmm2[1]
	unpcklps	%xmm14, %xmm2   ## xmm2 = xmm2[0],xmm14[0],xmm2[1],xmm14[1]
	unpcklps	%xmm10, %xmm2   ## xmm2 = xmm2[0],xmm10[0],xmm2[1],xmm10[1]
	mulps	%xmm2, %xmm5
	addps	%xmm3, %xmm5
	unpcklps	%xmm12, %xmm8   ## xmm8 = xmm8[0],xmm12[0],xmm8[1],xmm12[1]
	movss	40(%rbp), %xmm2         ## xmm2 = mem[0],zero,zero,zero
	unpcklps	%xmm2, %xmm1    ## xmm1 = xmm1[0],xmm2[0],xmm1[1],xmm2[1]
	unpcklps	%xmm1, %xmm1    ## xmm1 = xmm1[0,0,1,1]
	mulps	%xmm8, %xmm1
	unpcklps	%xmm13, %xmm7   ## xmm7 = xmm7[0],xmm13[0],xmm7[1],xmm13[1]
	movss	44(%rbp), %xmm3         ## xmm3 = mem[0],zero,zero,zero
	unpcklps	%xmm3, %xmm4    ## xmm4 = xmm4[0],xmm3[0],xmm4[1],xmm3[1]
	unpcklps	%xmm4, %xmm4    ## xmm4 = xmm4[0,0,1,1]
	mulps	%xmm7, %xmm4
	addps	%xmm1, %xmm4
	unpcklps	%xmm6, %xmm10   ## xmm10 = xmm10[0],xmm6[0],xmm10[1],xmm6[1]
	movss	48(%rbp), %xmm1         ## xmm1 = mem[0],zero,zero,zero
	unpcklps	%xmm1, %xmm0    ## xmm0 = xmm0[0],xmm1[0],xmm0[1],xmm1[1]
	unpcklps	%xmm0, %xmm0    ## xmm0 = xmm0[0,0,1,1]
	mulps	%xmm10, %xmm0
	addps	%xmm4, %xmm0
	mulss	%xmm2, %xmm9
	mulss	%xmm3, %xmm11
	addss	%xmm9, %xmm11
	mulss	%xmm1, %xmm14
	addss	%xmm11, %xmm14
	movups	%xmm5, (%rbx)
	movups	%xmm0, 16(%rbx)
	movss	%xmm14, 32(%rbx)
	movq	%rbx, %rax
	addq	$40, %rsp
	popq	%rbx
	popq	%rbp
	retq
	.cfi_endproc

	.section	__TEXT,__literal4,4byte_literals
	.align	2
LCPI6_0:
	.long	1065353216              ## float 1
	.section	__TEXT,__text,regular,pure_instructions
	.globl	_mat3_rotmat
	.align	4, 0x90
_mat3_rotmat:                           ## @mat3_rotmat
	.cfi_startproc
## BB#0:
	pushq	%rbp
Ltmp19:
	.cfi_def_cfa_offset 16
Ltmp20:
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
Ltmp21:
	.cfi_def_cfa_register %rbp
	pushq	%rbx
	subq	$24, %rsp
Ltmp22:
	.cfi_offset %rbx, -24
	movss	%xmm2, -12(%rbp)        ## 4-byte Spill
	movss	%xmm1, -16(%rbp)        ## 4-byte Spill
	movss	%xmm0, -20(%rbp)        ## 4-byte Spill
	movq	%rdi, %rbx
	xorps	%xmm0, %xmm0
	cvtss2sd	%xmm3, %xmm0
	callq	___sincos_stret
	cvtsd2ss	%xmm0, %xmm13
	cvtsd2ss	%xmm1, %xmm9
	movss	LCPI6_0(%rip), %xmm2    ## xmm2 = mem[0],zero,zero,zero
	subss	%xmm9, %xmm2
	movss	-20(%rbp), %xmm6        ## 4-byte Reload
                                        ## xmm6 = mem[0],zero,zero,zero
	movaps	%xmm6, %xmm3
	mulss	%xmm3, %xmm3
	mulss	%xmm2, %xmm3
	addss	%xmm9, %xmm3
	movaps	%xmm6, %xmm7
	movss	-16(%rbp), %xmm1        ## 4-byte Reload
                                        ## xmm1 = mem[0],zero,zero,zero
	mulss	%xmm1, %xmm7
	mulss	%xmm2, %xmm7
	movaps	%xmm13, %xmm4
	movss	-12(%rbp), %xmm0        ## 4-byte Reload
                                        ## xmm0 = mem[0],zero,zero,zero
	mulss	%xmm0, %xmm4
	movaps	%xmm7, %xmm8
	subss	%xmm4, %xmm8
	movaps	%xmm6, %xmm5
	mulss	%xmm0, %xmm5
	movaps	%xmm0, %xmm11
	mulss	%xmm2, %xmm5
	movaps	%xmm13, %xmm0
	mulss	%xmm1, %xmm0
	movaps	%xmm0, %xmm10
	addss	%xmm5, %xmm10
	addss	%xmm7, %xmm4
	movaps	%xmm1, %xmm7
	mulss	%xmm7, %xmm7
	mulss	%xmm2, %xmm7
	addss	%xmm9, %xmm7
	mulss	%xmm11, %xmm1
	mulss	%xmm2, %xmm1
	mulss	%xmm6, %xmm13
	movaps	%xmm1, %xmm12
	subss	%xmm13, %xmm12
	subss	%xmm0, %xmm5
	addss	%xmm1, %xmm13
	movaps	%xmm11, %xmm0
	mulss	%xmm0, %xmm0
	mulss	%xmm2, %xmm0
	addss	%xmm9, %xmm0
	movss	%xmm3, (%rbx)
	movss	%xmm8, 4(%rbx)
	movss	%xmm10, 8(%rbx)
	movss	%xmm4, 12(%rbx)
	movss	%xmm7, 16(%rbx)
	movss	%xmm12, 20(%rbx)
	movss	%xmm5, 24(%rbx)
	movss	%xmm13, 28(%rbx)
	movss	%xmm0, 32(%rbx)
	movq	%rbx, %rax
	addq	$24, %rsp
	popq	%rbx
	popq	%rbp
	retq
	.cfi_endproc

	.section	__TEXT,__literal16,16byte_literals
	.align	4
LCPI7_0:
	.long	2147483648              ## 0x80000000
	.long	2147483648              ## 0x80000000
	.long	2147483648              ## 0x80000000
	.long	2147483648              ## 0x80000000
	.section	__TEXT,__text,regular,pure_instructions
	.globl	_mat3_rotmatx
	.align	4, 0x90
_mat3_rotmatx:                          ## @mat3_rotmatx
	.cfi_startproc
## BB#0:
	pushq	%rbp
Ltmp23:
	.cfi_def_cfa_offset 16
Ltmp24:
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
Ltmp25:
	.cfi_def_cfa_register %rbp
	pushq	%rbx
	pushq	%rax
Ltmp26:
	.cfi_offset %rbx, -24
	movq	%rdi, %rbx
	cvtss2sd	%xmm0, %xmm0
	callq	___sincos_stret
	cvtsd2ss	%xmm0, %xmm0
	cvtsd2ss	%xmm1, %xmm1
	movss	LCPI7_0(%rip), %xmm2    ## xmm2 = mem[0],zero,zero,zero
	xorps	%xmm0, %xmm2
	movl	$1065353216, (%rbx)     ## imm = 0x3F800000
	movl	$0, 4(%rbx)
	movl	$0, 8(%rbx)
	movl	$0, 12(%rbx)
	movss	%xmm1, 16(%rbx)
	movss	%xmm2, 20(%rbx)
	movl	$0, 24(%rbx)
	movss	%xmm0, 28(%rbx)
	movss	%xmm1, 32(%rbx)
	movq	%rbx, %rax
	addq	$8, %rsp
	popq	%rbx
	popq	%rbp
	retq
	.cfi_endproc

	.section	__TEXT,__literal16,16byte_literals
	.align	4
LCPI8_0:
	.long	2147483648              ## 0x80000000
	.long	2147483648              ## 0x80000000
	.long	2147483648              ## 0x80000000
	.long	2147483648              ## 0x80000000
	.section	__TEXT,__text,regular,pure_instructions
	.globl	_mat3_rotmaty
	.align	4, 0x90
_mat3_rotmaty:                          ## @mat3_rotmaty
	.cfi_startproc
## BB#0:
	pushq	%rbp
Ltmp27:
	.cfi_def_cfa_offset 16
Ltmp28:
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
Ltmp29:
	.cfi_def_cfa_register %rbp
	pushq	%rbx
	pushq	%rax
Ltmp30:
	.cfi_offset %rbx, -24
	movq	%rdi, %rbx
	cvtss2sd	%xmm0, %xmm0
	callq	___sincos_stret
	cvtsd2ss	%xmm0, %xmm0
	cvtsd2ss	%xmm1, %xmm1
	movss	%xmm1, (%rbx)
	movl	$0, 4(%rbx)
	movss	%xmm0, 8(%rbx)
	xorps	LCPI8_0(%rip), %xmm0
	movl	$0, 12(%rbx)
	movl	$1065353216, 16(%rbx)   ## imm = 0x3F800000
	movl	$0, 20(%rbx)
	movss	%xmm0, 24(%rbx)
	movl	$0, 28(%rbx)
	movss	%xmm1, 32(%rbx)
	movq	%rbx, %rax
	addq	$8, %rsp
	popq	%rbx
	popq	%rbp
	retq
	.cfi_endproc

	.section	__TEXT,__literal16,16byte_literals
	.align	4
LCPI9_0:
	.long	2147483648              ## 0x80000000
	.long	2147483648              ## 0x80000000
	.long	2147483648              ## 0x80000000
	.long	2147483648              ## 0x80000000
	.section	__TEXT,__text,regular,pure_instructions
	.globl	_mat3_rotmatz
	.align	4, 0x90
_mat3_rotmatz:                          ## @mat3_rotmatz
	.cfi_startproc
## BB#0:
	pushq	%rbp
Ltmp31:
	.cfi_def_cfa_offset 16
Ltmp32:
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
Ltmp33:
	.cfi_def_cfa_register %rbp
	pushq	%rbx
	pushq	%rax
Ltmp34:
	.cfi_offset %rbx, -24
	movq	%rdi, %rbx
	cvtss2sd	%xmm0, %xmm0
	callq	___sincos_stret
	cvtsd2ss	%xmm0, %xmm0
	cvtsd2ss	%xmm1, %xmm1
	movss	LCPI9_0(%rip), %xmm2    ## xmm2 = mem[0],zero,zero,zero
	xorps	%xmm0, %xmm2
	movss	%xmm1, (%rbx)
	movss	%xmm2, 4(%rbx)
	movl	$0, 8(%rbx)
	movss	%xmm0, 12(%rbx)
	movss	%xmm1, 16(%rbx)
	movl	$0, 20(%rbx)
	movl	$0, 24(%rbx)
	movabsq	$4575657221408423936, %rax ## imm = 0x3F80000000000000
	movq	%rax, 28(%rbx)
	movq	%rbx, %rax
	addq	$8, %rsp
	popq	%rbx
	popq	%rbp
	retq
	.cfi_endproc

	.globl	_mat3_scale
	.align	4, 0x90
_mat3_scale:                            ## @mat3_scale
	.cfi_startproc
## BB#0:
	pushq	%rbp
Ltmp35:
	.cfi_def_cfa_offset 16
Ltmp36:
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
Ltmp37:
	.cfi_def_cfa_register %rbp
	mulss	16(%rbp), %xmm0
	movss	%xmm0, 16(%rbp)
	mulss	32(%rbp), %xmm1
	movss	%xmm1, 32(%rbp)
	mulss	44(%rbp), %xmm2
	movss	%xmm2, 44(%rbp)
	movl	48(%rbp), %eax
	movl	%eax, 32(%rdi)
	movq	40(%rbp), %rax
	movq	%rax, 24(%rdi)
	movq	32(%rbp), %rax
	movq	%rax, 16(%rdi)
	movq	16(%rbp), %rax
	movq	24(%rbp), %rcx
	movq	%rcx, 8(%rdi)
	movq	%rax, (%rdi)
	movq	%rdi, %rax
	popq	%rbp
	retq
	.cfi_endproc

	.globl	_mat3_scalemat
	.align	4, 0x90
_mat3_scalemat:                         ## @mat3_scalemat
	.cfi_startproc
## BB#0:
	pushq	%rbp
Ltmp38:
	.cfi_def_cfa_offset 16
Ltmp39:
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
Ltmp40:
	.cfi_def_cfa_register %rbp
	movss	%xmm0, (%rdi)
	movl	$0, 4(%rdi)
	movl	$0, 8(%rdi)
	movl	$0, 12(%rdi)
	movss	%xmm1, 16(%rdi)
	movl	$0, 20(%rdi)
	movl	$0, 24(%rdi)
	movl	$0, 28(%rdi)
	movss	%xmm2, 32(%rdi)
	movq	%rdi, %rax
	popq	%rbp
	retq
	.cfi_endproc

	.globl	_mat3_transp
	.align	4, 0x90
_mat3_transp:                           ## @mat3_transp
	.cfi_startproc
## BB#0:
	pushq	%rbp
Ltmp41:
	.cfi_def_cfa_offset 16
Ltmp42:
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
Ltmp43:
	.cfi_def_cfa_register %rbp
	movq	16(%rbp), %rax
	movl	28(%rbp), %ecx
	movq	40(%rbp), %rdx
	movl	%eax, (%rdi)
	shrq	$32, %rax
	movq	32(%rbp), %rsi
	movl	%ecx, 4(%rdi)
	movl	%edx, 8(%rdi)
	shrq	$32, %rdx
	movl	24(%rbp), %ecx
	movl	%eax, 12(%rdi)
	movl	%esi, 16(%rdi)
	shrq	$32, %rsi
	movl	48(%rbp), %eax
	movl	%edx, 20(%rdi)
	movl	%ecx, 24(%rdi)
	movl	%esi, 28(%rdi)
	movl	%eax, 32(%rdi)
	movq	%rdi, %rax
	popq	%rbp
	retq
	.cfi_endproc

	.globl	_mat3_lookat
	.align	4, 0x90
_mat3_lookat:                           ## @mat3_lookat
	.cfi_startproc
## BB#0:
	pushq	%rbp
Ltmp44:
	.cfi_def_cfa_offset 16
Ltmp45:
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
Ltmp46:
	.cfi_def_cfa_register %rbp
	pushq	%rbx
	subq	$72, %rsp
Ltmp47:
	.cfi_offset %rbx, -24
	movss	%xmm5, -52(%rbp)        ## 4-byte Spill
	movaps	%xmm4, -80(%rbp)        ## 16-byte Spill
	movq	%rdi, %rbx
	callq	_v3_sub
	callq	_v3_normalize
	movaps	%xmm0, %xmm2
	movaps	%xmm2, -32(%rbp)        ## 16-byte Spill
	movaps	%xmm1, %xmm3
	movss	%xmm3, -12(%rbp)        ## 4-byte Spill
	movshdup	%xmm2, %xmm0    ## xmm0 = xmm2[1,1,3,3]
	movaps	%xmm0, -48(%rbp)        ## 16-byte Spill
	movaps	-80(%rbp), %xmm0        ## 16-byte Reload
	movss	-52(%rbp), %xmm1        ## 4-byte Reload
                                        ## xmm1 = mem[0],zero,zero,zero
	callq	_v3_cross
	callq	_v3_normalize
	movaps	%xmm0, %xmm2
	movaps	%xmm2, -80(%rbp)        ## 16-byte Spill
	movaps	%xmm1, %xmm3
	movss	%xmm3, -52(%rbp)        ## 4-byte Spill
	movaps	-32(%rbp), %xmm0        ## 16-byte Reload
	movss	-12(%rbp), %xmm1        ## 4-byte Reload
                                        ## xmm1 = mem[0],zero,zero,zero
	callq	_v3_cross
	movaps	-80(%rbp), %xmm4        ## 16-byte Reload
	movshdup	%xmm4, %xmm2    ## xmm2 = xmm4[1,1,3,3]
	movshdup	%xmm0, %xmm3    ## xmm3 = xmm0[1,1,3,3]
	movss	%xmm4, (%rbx)
	movss	%xmm0, 4(%rbx)
	movaps	-32(%rbp), %xmm0        ## 16-byte Reload
	movss	%xmm0, 8(%rbx)
	movss	%xmm2, 12(%rbx)
	movss	%xmm3, 16(%rbx)
	movaps	-48(%rbp), %xmm0        ## 16-byte Reload
	movss	%xmm0, 20(%rbx)
	movss	-52(%rbp), %xmm0        ## 4-byte Reload
                                        ## xmm0 = mem[0],zero,zero,zero
	movss	%xmm0, 24(%rbx)
	movss	%xmm1, 28(%rbx)
	movss	-12(%rbp), %xmm0        ## 4-byte Reload
                                        ## xmm0 = mem[0],zero,zero,zero
	movss	%xmm0, 32(%rbx)
	movq	%rbx, %rax
	addq	$72, %rsp
	popq	%rbx
	popq	%rbp
	retq
	.cfi_endproc

	.globl	_mat3_v3_to_array
	.align	4, 0x90
_mat3_v3_to_array:                      ## @mat3_v3_to_array
	.cfi_startproc
## BB#0:
	pushq	%rbp
Ltmp48:
	.cfi_def_cfa_offset 16
Ltmp49:
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
Ltmp50:
	.cfi_def_cfa_register %rbp
	subq	$80, %rsp
	movq	___stack_chk_guard@GOTPCREL(%rip), %rax
	movq	(%rax), %rax
	movq	%rax, -8(%rbp)
	cmpl	$16, %esi
	jne	LBB14_3
## BB#1:
	leaq	16(%rbp), %rcx
	movq	(%rcx), %rdx
	movl	%edx, -80(%rbp)
	shrq	$32, %rdx
	movl	%edx, -76(%rbp)
	movq	8(%rcx), %rdx
	movl	%edx, -72(%rbp)
	movss	%xmm0, -68(%rbp)
	shrq	$32, %rdx
	movl	%edx, -64(%rbp)
	movq	16(%rcx), %rdx
	movl	%edx, -60(%rbp)
	shrq	$32, %rdx
	movl	%edx, -56(%rbp)
	movshdup	%xmm0, %xmm0    ## xmm0 = xmm0[1,1,3,3]
	movss	%xmm0, -52(%rbp)
	movq	24(%rcx), %rdx
	movl	%edx, -48(%rbp)
	shrq	$32, %rdx
	movl	%edx, -44(%rbp)
	movl	32(%rcx), %ecx
	movl	%ecx, -40(%rbp)
	movss	%xmm1, -36(%rbp)
	movl	$0, -32(%rbp)
	movl	$0, -28(%rbp)
	movl	$0, -24(%rbp)
	movl	$1065353216, -20(%rbp)  ## imm = 0x3F800000
	movq	-24(%rbp), %rcx
	movq	%rcx, 56(%rdi)
	movq	-32(%rbp), %rcx
	movq	%rcx, 48(%rdi)
	movq	-40(%rbp), %rcx
	movq	%rcx, 40(%rdi)
	movq	-48(%rbp), %rcx
	movq	%rcx, 32(%rdi)
	movq	-56(%rbp), %rcx
	movq	%rcx, 24(%rdi)
	movq	-64(%rbp), %rcx
	movq	%rcx, 16(%rdi)
	movq	-80(%rbp), %rcx
	movq	-72(%rbp), %rdx
	movq	%rdx, 8(%rdi)
	movq	%rcx, (%rdi)
	cmpq	-8(%rbp), %rax
	jne	LBB14_4
## BB#2:
	addq	$80, %rsp
	popq	%rbp
	retq
LBB14_3:
	leaq	L___func__.mat3_v3_to_array(%rip), %rdi
	leaq	L_.str(%rip), %rsi
	leaq	L_.str2(%rip), %rcx
	movl	$180, %edx
	callq	___assert_rtn
LBB14_4:
	callq	___stack_chk_fail
	.cfi_endproc

	.section	__TEXT,__cstring,cstring_literals
L___func__.mat3_to_array:               ## @__func__.mat3_to_array
	.asciz	"mat3_to_array"

L_.str:                                 ## @.str
	.asciz	"matrix3.c"

L_.str1:                                ## @.str1
	.asciz	"len == 9"

L___func__.mat3_v3_to_array:            ## @__func__.mat3_v3_to_array
	.asciz	"mat3_v3_to_array"

L_.str2:                                ## @.str2
	.asciz	"len == 16"


.subsections_via_symbols
