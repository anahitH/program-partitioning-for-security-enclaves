	.text
	.file	"snake.bc"
	.globl	setup_level             # -- Begin function setup_level
	.p2align	4, 0x90
	.type	setup_level,@function
setup_level:                            # @setup_level
	.cfi_startproc
# %bb.0:                                # %entry
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register %rbp
	subq	$48, %rsp
	movq	%rdi, -24(%rbp)
	movq	%rsi, -40(%rbp)
	movl	%edx, -28(%rbp)
	xorl	%edi, %edi
	callq	insecure_time
	movl	%eax, %edi
	callq	insecure_srand
	movl	$1, %eax
	cmpl	-28(%rbp), %eax
	jne	.LBB0_2
# %bb.1:                                # %if.then
	movq	-24(%rbp), %rax
	movl	$0, 4(%rax)
	movq	-24(%rbp), %rax
	movl	$4, 16(%rax)
	movq	-24(%rbp), %rax
	movl	$1, (%rax)
	movq	-40(%rbp), %rax
	movl	$14, (%rax)
	movq	-40(%rbp), %rax
	movl	$1, 4(%rax)
	jmp	.LBB0_6
.LBB0_2:                                # %if.else
	movl	$5, %ecx
	movq	-24(%rbp), %rax
	imull	$1000, (%rax), %eax     # imm = 0x3E8
	movq	-24(%rbp), %rdx
	addl	4(%rdx), %eax
	movl	%eax, 4(%rdx)
	movq	-24(%rbp), %rax
	movl	16(%rax), %edx
	addl	$2, %edx
	movl	%edx, 16(%rax)
	movq	-24(%rbp), %rax
	movl	(%rax), %edx
	addl	$1, %edx
	movl	%edx, (%rax)
	movq	-24(%rbp), %rax
	movl	(%rax), %eax
	cltd
	idivl	%ecx
	cmpl	$0, %edx
	jne	.LBB0_5
# %bb.3:                                # %land.lhs.true
	movq	-40(%rbp), %rax
	cmpl	$1, (%rax)
	jbe	.LBB0_5
# %bb.4:                                # %if.then14
	movq	-40(%rbp), %rax
	movl	(%rax), %ecx
	addl	$-1, %ecx
	movl	%ecx, (%rax)
.LBB0_5:                                # %if.end
	jmp	.LBB0_6
.LBB0_6:                                # %if.end16
	movl	$200000, %eax           # imm = 0x30D40
	movq	-24(%rbp), %rcx
	movl	$0, 12(%rcx)
	movl	-28(%rbp), %ecx
	addl	$4, %ecx
	movq	-40(%rbp), %rdx
	movl	%ecx, 8(%rdx)
	imull	$10000, -28(%rbp), %ecx # imm = 0x2710
	subl	%ecx, %eax
	movl	%eax, usec_delay
	movl	usec_delay, %edi
	callq	set_app_usec_delay
	movl	$0, -4(%rbp)
.LBB0_7:                                # %for.cond
                                        # =>This Loop Header: Depth=1
                                        #     Child Loop BB0_9 Depth 2
	cmpl	$22, -4(%rbp)
	jge	.LBB0_14
# %bb.8:                                # %for.body
                                        #   in Loop: Header=BB0_7 Depth=1
	movl	$0, -8(%rbp)
.LBB0_9:                                # %for.cond21
                                        #   Parent Loop BB0_7 Depth=1
                                        # =>  This Inner Loop Header: Depth=2
	cmpl	$78, -8(%rbp)
	jge	.LBB0_12
# %bb.10:                               # %for.body24
                                        #   in Loop: Header=BB0_9 Depth=2
	movq	-24(%rbp), %rax
	addq	$20, %rax
	movslq	-4(%rbp), %rcx
	imulq	$78, %rcx, %rcx
	addq	%rcx, %rax
	movslq	-8(%rbp), %rcx
	movb	$32, (%rax,%rcx)
# %bb.11:                               # %for.inc
                                        #   in Loop: Header=BB0_9 Depth=2
	movl	-8(%rbp), %eax
	addl	$1, %eax
	movl	%eax, -8(%rbp)
	jmp	.LBB0_9
.LBB0_12:                               # %for.end
                                        #   in Loop: Header=BB0_7 Depth=1
	jmp	.LBB0_13
.LBB0_13:                               # %for.inc28
                                        #   in Loop: Header=BB0_7 Depth=1
	movl	-4(%rbp), %eax
	addl	$1, %eax
	movl	%eax, -4(%rbp)
	jmp	.LBB0_7
.LBB0_14:                               # %for.end30
	movl	$0, -12(%rbp)
.LBB0_15:                               # %for.cond31
                                        # =>This Loop Header: Depth=1
                                        #     Child Loop BB0_17 Depth 2
	movl	-12(%rbp), %eax
	movq	-24(%rbp), %rcx
	movl	16(%rcx), %ecx
	shll	$1, %ecx
	cmpl	%ecx, %eax
	jge	.LBB0_24
# %bb.16:                               # %for.body36
                                        #   in Loop: Header=BB0_15 Depth=1
	jmp	.LBB0_17
.LBB0_17:                               # %do.body
                                        #   Parent Loop BB0_15 Depth=1
                                        # =>  This Inner Loop Header: Depth=2
	callq	insecure_rand
	movl	$22, %ecx
	cltd
	idivl	%ecx
	movl	%edx, -4(%rbp)
	callq	insecure_rand
	movl	$78, %ecx
	cltd
	idivl	%ecx
	movl	%edx, -8(%rbp)
# %bb.18:                               # %do.cond
                                        #   in Loop: Header=BB0_17 Depth=2
	movq	-24(%rbp), %rax
	addq	$20, %rax
	movslq	-4(%rbp), %rcx
	imulq	$78, %rcx, %rcx
	addq	%rcx, %rax
	movslq	-8(%rbp), %rcx
	movsbl	(%rax,%rcx), %eax
	cmpl	$32, %eax
	jne	.LBB0_17
# %bb.19:                               # %do.end
                                        #   in Loop: Header=BB0_15 Depth=1
	movl	-12(%rbp), %eax
	movq	-24(%rbp), %rcx
	cmpl	16(%rcx), %eax
	jge	.LBB0_21
# %bb.20:                               # %if.then52
                                        #   in Loop: Header=BB0_15 Depth=1
	movq	-24(%rbp), %rax
	addq	$20, %rax
	movslq	-4(%rbp), %rcx
	imulq	$78, %rcx, %rcx
	addq	%rcx, %rax
	movslq	-8(%rbp), %rcx
	movb	$42, (%rax,%rcx)
	jmp	.LBB0_22
.LBB0_21:                               # %if.else58
                                        #   in Loop: Header=BB0_15 Depth=1
	movq	-24(%rbp), %rax
	movl	12(%rax), %ecx
	addl	$1, %ecx
	movl	%ecx, 12(%rax)
	movq	-24(%rbp), %rax
	addq	$20, %rax
	movslq	-4(%rbp), %rcx
	imulq	$78, %rcx, %rcx
	addq	%rcx, %rax
	movslq	-8(%rbp), %rcx
	movb	$36, (%rax,%rcx)
.LBB0_22:                               # %if.end66
                                        #   in Loop: Header=BB0_15 Depth=1
	jmp	.LBB0_23
.LBB0_23:                               # %for.inc67
                                        #   in Loop: Header=BB0_15 Depth=1
	movl	-12(%rbp), %eax
	addl	$1, %eax
	movl	%eax, -12(%rbp)
	jmp	.LBB0_15
.LBB0_24:                               # %for.end69
	movl	$0, -12(%rbp)
.LBB0_25:                               # %for.cond70
                                        # =>This Inner Loop Header: Depth=1
	movl	-12(%rbp), %eax
	movq	-40(%rbp), %rcx
	cmpl	8(%rcx), %eax
	jge	.LBB0_31
# %bb.26:                               # %for.body74
                                        #   in Loop: Header=BB0_25 Depth=1
	movq	-40(%rbp), %rax
	movslq	-12(%rbp), %rcx
	movl	$11, 12(%rax,%rcx,8)
	movq	-40(%rbp), %rax
	cmpl	$0, 4(%rax)
	jne	.LBB0_28
# %bb.27:                               # %cond.true
                                        #   in Loop: Header=BB0_25 Depth=1
	movl	$39, %eax
	movl	-28(%rbp), %ecx
	addl	$4, %ecx
	subl	%ecx, %eax
	subl	-12(%rbp), %eax
	jmp	.LBB0_29
.LBB0_28:                               # %cond.false
                                        #   in Loop: Header=BB0_25 Depth=1
	movl	$39, %eax
	movl	-28(%rbp), %ecx
	addl	$4, %ecx
	subl	%ecx, %eax
	addl	-12(%rbp), %eax
.LBB0_29:                               # %cond.end
                                        #   in Loop: Header=BB0_25 Depth=1
	movq	-40(%rbp), %rcx
	movslq	-12(%rbp), %rdx
	movl	%eax, 16(%rcx,%rdx,8)
# %bb.30:                               # %for.inc91
                                        #   in Loop: Header=BB0_25 Depth=1
	movl	-12(%rbp), %eax
	addl	$1, %eax
	movl	%eax, -12(%rbp)
	jmp	.LBB0_25
.LBB0_31:                               # %for.end93
	movabsq	$.L.str, %rdi
	callq	insecure_puts
	movl	$1, %edi
	movl	$1, %esi
	callq	insecure_draw_line
	movl	$0, -4(%rbp)
.LBB0_32:                               # %for.cond95
                                        # =>This Loop Header: Depth=1
                                        #     Child Loop BB0_34 Depth 2
	cmpl	$22, -4(%rbp)
	jge	.LBB0_39
# %bb.33:                               # %for.body98
                                        #   in Loop: Header=BB0_32 Depth=1
	movabsq	$.L.str.3, %rdi
	movl	-4(%rbp), %esi
	addl	$2, %esi
	movl	$1, %edx
	movb	$0, %al
	callq	insecure_printf
	movabsq	$.L.str.2, %rdi
	movl	$1, %esi
	movl	$34, %edx
	movb	$0, %al
	callq	insecure_printf
	movabsq	$.L.str.2, %rdi
	movl	$1, %esi
	movl	$44, %edx
	movb	$0, %al
	callq	insecure_printf
	movabsq	$.L.str.11, %rdi
	movb	$0, %al
	callq	insecure_printf
	movabsq	$.L.str.10, %rdi
	xorl	%esi, %esi
	movb	$0, %al
	callq	insecure_printf
	movabsq	$.L.str.2, %rdi
	movl	$1, %esi
	movl	$37, %edx
	movb	$0, %al
	callq	insecure_printf
	movl	$0, -8(%rbp)
.LBB0_34:                               # %for.cond106
                                        #   Parent Loop BB0_32 Depth=1
                                        # =>  This Inner Loop Header: Depth=2
	cmpl	$78, -8(%rbp)
	jge	.LBB0_37
# %bb.35:                               # %for.body109
                                        #   in Loop: Header=BB0_34 Depth=2
	movabsq	$.L.str.12, %rdi
	movq	-24(%rbp), %rax
	addq	$20, %rax
	movslq	-4(%rbp), %rcx
	imulq	$78, %rcx, %rcx
	addq	%rcx, %rax
	movslq	-8(%rbp), %rcx
	movsbl	(%rax,%rcx), %esi
	movb	$0, %al
	callq	insecure_printf
# %bb.36:                               # %for.inc117
                                        #   in Loop: Header=BB0_34 Depth=2
	movl	-8(%rbp), %eax
	addl	$1, %eax
	movl	%eax, -8(%rbp)
	jmp	.LBB0_34
.LBB0_37:                               # %for.end119
                                        #   in Loop: Header=BB0_32 Depth=1
	movabsq	$.L.str.2, %rdi
	movl	$1, %esi
	movl	$34, %edx
	movb	$0, %al
	callq	insecure_printf
	movabsq	$.L.str.2, %rdi
	movl	$1, %esi
	movl	$44, %edx
	movb	$0, %al
	callq	insecure_printf
	movabsq	$.L.str.11, %rdi
	movb	$0, %al
	callq	insecure_printf
	movabsq	$.L.str.10, %rdi
	xorl	%esi, %esi
	movb	$0, %al
	callq	insecure_printf
# %bb.38:                               # %for.inc124
                                        #   in Loop: Header=BB0_32 Depth=1
	movl	-4(%rbp), %eax
	addl	$1, %eax
	movl	%eax, -4(%rbp)
	jmp	.LBB0_32
.LBB0_39:                               # %for.end126
	movl	$1, %edi
	movl	$24, %esi
	callq	insecure_draw_line
	movq	-24(%rbp), %rdi
	callq	insecure_show_score
	movabsq	$.L.str.2, %rdi
	movl	$1, %esi
	movl	$31, %edx
	movb	$0, %al
	callq	insecure_printf
	movabsq	$.L.str.3, %rdi
	movl	$1, %esi
	movl	$30, %edx
	movb	$0, %al
	callq	insecure_printf
	addq	$48, %rsp
	popq	%rbp
	retq
.Lfunc_end0:
	.size	setup_level, .Lfunc_end0-setup_level
	.cfi_endproc
                                        # -- End function
	.globl	move                    # -- Begin function move
	.p2align	4, 0x90
	.type	move,@function
move:                                   # @move
	.cfi_startproc
# %bb.0:                                # %entry
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register %rbp
	subq	$48, %rsp
	movq	%rdi, -16(%rbp)
	movq	%rsi, -32(%rbp)
	movb	%dl, -1(%rbp)
	movq	-16(%rbp), %rax
	movl	4(%rax), %eax
	movl	%eax, -36(%rbp)
	movsbl	-1(%rbp), %eax
	movq	-32(%rbp), %rcx
	movsbl	1(%rcx), %ecx
	cmpl	%ecx, %eax
	jne	.LBB1_2
# %bb.1:                                # %if.then
	movq	-16(%rbp), %rax
	movl	$1, 4(%rax)
	jmp	.LBB1_31
.LBB1_2:                                # %if.else
	movsbl	-1(%rbp), %eax
	movq	-32(%rbp), %rcx
	movsbl	(%rcx), %ecx
	cmpl	%ecx, %eax
	jne	.LBB1_4
# %bb.3:                                # %if.then9
	movq	-16(%rbp), %rax
	movl	$0, 4(%rax)
	jmp	.LBB1_30
.LBB1_4:                                # %if.else11
	movsbl	-1(%rbp), %eax
	movq	-32(%rbp), %rcx
	movsbl	2(%rcx), %ecx
	cmpl	%ecx, %eax
	jne	.LBB1_6
# %bb.5:                                # %if.then17
	movq	-16(%rbp), %rax
	movl	$2, 4(%rax)
	jmp	.LBB1_29
.LBB1_6:                                # %if.else19
	movsbl	-1(%rbp), %eax
	movq	-32(%rbp), %rcx
	movsbl	3(%rcx), %ecx
	cmpl	%ecx, %eax
	jne	.LBB1_8
# %bb.7:                                # %if.then25
	movq	-16(%rbp), %rax
	movl	$3, 4(%rax)
	jmp	.LBB1_28
.LBB1_8:                                # %if.else27
	movsbl	-1(%rbp), %eax
	movq	-32(%rbp), %rcx
	movsbl	4(%rcx), %ecx
	cmpl	%ecx, %eax
	jne	.LBB1_17
# %bb.9:                                # %if.then33
	movl	-36(%rbp), %eax
	movq	%rax, %rcx
	subq	$3, %rcx
	ja	.LBB1_15
# %bb.10:                               # %if.then33
	movq	.LJTI1_1(,%rax,8), %rax
	jmpq	*%rax
.LBB1_11:                               # %sw.bb
	movq	-16(%rbp), %rax
	movl	$3, 4(%rax)
	jmp	.LBB1_16
.LBB1_12:                               # %sw.bb35
	movq	-16(%rbp), %rax
	movl	$2, 4(%rax)
	jmp	.LBB1_16
.LBB1_13:                               # %sw.bb37
	movq	-16(%rbp), %rax
	movl	$0, 4(%rax)
	jmp	.LBB1_16
.LBB1_14:                               # %sw.bb39
	movq	-16(%rbp), %rax
	movl	$1, 4(%rax)
	jmp	.LBB1_16
.LBB1_15:                               # %sw.default
	jmp	.LBB1_16
.LBB1_16:                               # %sw.epilog
	jmp	.LBB1_27
.LBB1_17:                               # %if.else41
	movsbl	-1(%rbp), %eax
	movq	-32(%rbp), %rcx
	movsbl	5(%rcx), %ecx
	cmpl	%ecx, %eax
	jne	.LBB1_26
# %bb.18:                               # %if.then47
	movl	-36(%rbp), %eax
	movq	%rax, %rcx
	subq	$3, %rcx
	ja	.LBB1_24
# %bb.19:                               # %if.then47
	movq	.LJTI1_0(,%rax,8), %rax
	jmpq	*%rax
.LBB1_20:                               # %sw.bb48
	movq	-16(%rbp), %rax
	movl	$2, 4(%rax)
	jmp	.LBB1_25
.LBB1_21:                               # %sw.bb50
	movq	-16(%rbp), %rax
	movl	$3, 4(%rax)
	jmp	.LBB1_25
.LBB1_22:                               # %sw.bb52
	movq	-16(%rbp), %rax
	movl	$1, 4(%rax)
	jmp	.LBB1_25
.LBB1_23:                               # %sw.bb54
	movq	-16(%rbp), %rax
	movl	$0, 4(%rax)
	jmp	.LBB1_25
.LBB1_24:                               # %sw.default56
	jmp	.LBB1_25
.LBB1_25:                               # %sw.epilog57
	jmp	.LBB1_26
.LBB1_26:                               # %if.end
	jmp	.LBB1_27
.LBB1_27:                               # %if.end58
	jmp	.LBB1_28
.LBB1_28:                               # %if.end59
	jmp	.LBB1_29
.LBB1_29:                               # %if.end60
	jmp	.LBB1_30
.LBB1_30:                               # %if.end61
	jmp	.LBB1_31
.LBB1_31:                               # %if.end62
	movq	-16(%rbp), %rax
	movl	4(%rax), %eax
	movq	%rax, %rcx
	subq	$3, %rcx
	ja	.LBB1_37
# %bb.32:                               # %if.end62
	movq	.LJTI1_2(,%rax,8), %rax
	jmpq	*%rax
.LBB1_33:                               # %sw.bb64
	movq	-16(%rbp), %rax
	movq	-16(%rbp), %rcx
	movl	8(%rcx), %ecx
	subl	$1, %ecx
	movslq	%ecx, %rcx
	movl	12(%rax,%rcx,8), %eax
	movq	-16(%rbp), %rcx
	movq	-16(%rbp), %rdx
	movslq	8(%rdx), %rdx
	movl	%eax, 12(%rcx,%rdx,8)
	movq	-16(%rbp), %rax
	movq	-16(%rbp), %rcx
	movl	8(%rcx), %ecx
	subl	$1, %ecx
	movslq	%ecx, %rcx
	movl	16(%rax,%rcx,8), %eax
	subl	$1, %eax
	movq	-16(%rbp), %rcx
	movq	-16(%rbp), %rdx
	movslq	8(%rdx), %rdx
	movl	%eax, 16(%rcx,%rdx,8)
	jmp	.LBB1_38
.LBB1_34:                               # %sw.bb82
	movq	-16(%rbp), %rax
	movq	-16(%rbp), %rcx
	movl	8(%rcx), %ecx
	subl	$1, %ecx
	movslq	%ecx, %rcx
	movl	12(%rax,%rcx,8), %eax
	movq	-16(%rbp), %rcx
	movq	-16(%rbp), %rdx
	movslq	8(%rdx), %rdx
	movl	%eax, 12(%rcx,%rdx,8)
	movq	-16(%rbp), %rax
	movq	-16(%rbp), %rcx
	movl	8(%rcx), %ecx
	subl	$1, %ecx
	movslq	%ecx, %rcx
	movl	16(%rax,%rcx,8), %eax
	addl	$1, %eax
	movq	-16(%rbp), %rcx
	movq	-16(%rbp), %rdx
	movslq	8(%rdx), %rdx
	movl	%eax, 16(%rcx,%rdx,8)
	jmp	.LBB1_38
.LBB1_35:                               # %sw.bb105
	movq	-16(%rbp), %rax
	movq	-16(%rbp), %rcx
	movl	8(%rcx), %ecx
	subl	$1, %ecx
	movslq	%ecx, %rcx
	movl	12(%rax,%rcx,8), %eax
	subl	$1, %eax
	movq	-16(%rbp), %rcx
	movq	-16(%rbp), %rdx
	movslq	8(%rdx), %rdx
	movl	%eax, 12(%rcx,%rdx,8)
	movq	-16(%rbp), %rax
	movq	-16(%rbp), %rcx
	movl	8(%rcx), %ecx
	subl	$1, %ecx
	movslq	%ecx, %rcx
	movl	16(%rax,%rcx,8), %eax
	movq	-16(%rbp), %rcx
	movq	-16(%rbp), %rdx
	movslq	8(%rdx), %rdx
	movl	%eax, 16(%rcx,%rdx,8)
	jmp	.LBB1_38
.LBB1_36:                               # %sw.bb129
	movq	-16(%rbp), %rax
	movq	-16(%rbp), %rcx
	movl	8(%rcx), %ecx
	subl	$1, %ecx
	movslq	%ecx, %rcx
	movl	12(%rax,%rcx,8), %eax
	addl	$1, %eax
	movq	-16(%rbp), %rcx
	movq	-16(%rbp), %rdx
	movslq	8(%rdx), %rdx
	movl	%eax, 12(%rcx,%rdx,8)
	movq	-16(%rbp), %rax
	movq	-16(%rbp), %rcx
	movl	8(%rcx), %ecx
	subl	$1, %ecx
	movslq	%ecx, %rcx
	movl	16(%rax,%rcx,8), %eax
	movq	-16(%rbp), %rcx
	movq	-16(%rbp), %rdx
	movslq	8(%rdx), %rdx
	movl	%eax, 16(%rcx,%rdx,8)
	jmp	.LBB1_38
.LBB1_37:                               # %sw.default153
	jmp	.LBB1_38
.LBB1_38:                               # %sw.epilog154
	movabsq	$.L.str.10, %rdi
	xorl	%esi, %esi
	movb	$0, %al
	callq	insecure_printf
	movabsq	$.L.str.3, %rdi
	movq	-16(%rbp), %rax
	movl	12(%rax), %esi
	addl	$1, %esi
	movq	-16(%rbp), %rax
	movl	16(%rax), %edx
	addl	$1, %edx
	movb	$0, %al
	callq	insecure_printf
	movabsq	$.L.str.13, %rdi
	callq	insecure_puts
	movl	$1, -20(%rbp)
.LBB1_39:                               # %for.cond
                                        # =>This Inner Loop Header: Depth=1
	movl	-20(%rbp), %eax
	movq	-16(%rbp), %rcx
	cmpl	8(%rcx), %eax
	jg	.LBB1_42
# %bb.40:                               # %for.body
                                        #   in Loop: Header=BB1_39 Depth=1
	movq	-16(%rbp), %rdi
	addq	$12, %rdi
	movl	-20(%rbp), %eax
	subl	$1, %eax
	cltq
	shlq	$3, %rax
	addq	%rax, %rdi
	movq	-16(%rbp), %rsi
	addq	$12, %rsi
	movslq	-20(%rbp), %rax
	shlq	$3, %rax
	addq	%rax, %rsi
	movl	$8, %edx
	movl	$4, %ecx
	xorl	%r8d, %r8d
	callq	insecure_llvm.memcpy.p0i8.p0i8.i64
# %bb.41:                               # %for.inc
                                        #   in Loop: Header=BB1_39 Depth=1
	movl	-20(%rbp), %eax
	addl	$1, %eax
	movl	%eax, -20(%rbp)
	jmp	.LBB1_39
.LBB1_42:                               # %for.end
	movabsq	$.L.str.2, %rdi
	movl	$1, %esi
	movl	$43, %edx
	movb	$0, %al
	callq	insecure_printf
	movl	$0, -20(%rbp)
.LBB1_43:                               # %for.cond176
                                        # =>This Inner Loop Header: Depth=1
	movl	-20(%rbp), %eax
	movq	-16(%rbp), %rcx
	cmpl	8(%rcx), %eax
	jge	.LBB1_46
# %bb.44:                               # %for.body180
                                        #   in Loop: Header=BB1_43 Depth=1
	movabsq	$.L.str.3, %rdi
	movq	-16(%rbp), %rax
	movslq	-20(%rbp), %rcx
	movl	12(%rax,%rcx,8), %esi
	addl	$1, %esi
	movq	-16(%rbp), %rax
	movslq	-20(%rbp), %rcx
	movl	16(%rax,%rcx,8), %edx
	addl	$1, %edx
	movb	$0, %al
	callq	insecure_printf
	movabsq	$.L.str.13, %rdi
	callq	insecure_puts
# %bb.45:                               # %for.inc193
                                        #   in Loop: Header=BB1_43 Depth=1
	movl	-20(%rbp), %eax
	addl	$1, %eax
	movl	%eax, -20(%rbp)
	jmp	.LBB1_43
.LBB1_46:                               # %for.end195
	movabsq	$.L.str.10, %rdi
	xorl	%esi, %esi
	movb	$0, %al
	callq	insecure_printf
	addq	$48, %rsp
	popq	%rbp
	retq
.Lfunc_end1:
	.size	move, .Lfunc_end1-move
	.cfi_endproc
	.section	.rodata,"a",@progbits
	.p2align	3
.LJTI1_0:
	.quad	.LBB1_20
	.quad	.LBB1_21
	.quad	.LBB1_22
	.quad	.LBB1_23
.LJTI1_1:
	.quad	.LBB1_11
	.quad	.LBB1_12
	.quad	.LBB1_13
	.quad	.LBB1_14
.LJTI1_2:
	.quad	.LBB1_33
	.quad	.LBB1_34
	.quad	.LBB1_35
	.quad	.LBB1_36
                                        # -- End function
	.text
	.globl	collide_walls           # -- Begin function collide_walls
	.p2align	4, 0x90
	.type	collide_walls,@function
collide_walls:                          # @collide_walls
	.cfi_startproc
# %bb.0:                                # %entry
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register %rbp
	movq	%rdi, -24(%rbp)
	movq	-24(%rbp), %rax
	addq	$12, %rax
	movq	-24(%rbp), %rcx
	movl	8(%rcx), %ecx
	subl	$1, %ecx
	movslq	%ecx, %rcx
	shlq	$3, %rcx
	addq	%rcx, %rax
	movq	%rax, -16(%rbp)
	movq	-16(%rbp), %rax
	cmpl	$22, (%rax)
	jg	.LBB2_4
# %bb.1:                                # %lor.lhs.false
	movq	-16(%rbp), %rax
	cmpl	$1, (%rax)
	jl	.LBB2_4
# %bb.2:                                # %lor.lhs.false3
	movq	-16(%rbp), %rax
	cmpl	$78, 4(%rax)
	jg	.LBB2_4
# %bb.3:                                # %lor.lhs.false5
	movq	-16(%rbp), %rax
	cmpl	$1, 4(%rax)
	jge	.LBB2_5
.LBB2_4:                                # %if.then
	movl	$1, -4(%rbp)
	jmp	.LBB2_6
.LBB2_5:                                # %if.end
	movl	$0, -4(%rbp)
.LBB2_6:                                # %return
	movl	-4(%rbp), %eax
	popq	%rbp
	retq
.Lfunc_end2:
	.size	collide_walls, .Lfunc_end2-collide_walls
	.cfi_endproc
                                        # -- End function
	.globl	collide_object          # -- Begin function collide_object
	.p2align	4, 0x90
	.type	collide_object,@function
collide_object:                         # @collide_object
	.cfi_startproc
# %bb.0:                                # %entry
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register %rbp
	movq	%rdi, -24(%rbp)
	movq	%rsi, -32(%rbp)
	movb	%dl, -1(%rbp)
	movq	-24(%rbp), %rax
	addq	$12, %rax
	movq	-24(%rbp), %rcx
	movl	8(%rcx), %ecx
	subl	$1, %ecx
	movslq	%ecx, %rcx
	shlq	$3, %rcx
	addq	%rcx, %rax
	movq	%rax, -16(%rbp)
	movq	-32(%rbp), %rax
	addq	$20, %rax
	movq	-16(%rbp), %rcx
	movl	(%rcx), %ecx
	subl	$1, %ecx
	movslq	%ecx, %rcx
	imulq	$78, %rcx, %rcx
	addq	%rcx, %rax
	movq	-16(%rbp), %rcx
	movl	4(%rcx), %ecx
	subl	$1, %ecx
	movslq	%ecx, %rcx
	movsbl	(%rax,%rcx), %eax
	movsbl	-1(%rbp), %ecx
	cmpl	%ecx, %eax
	jne	.LBB3_2
# %bb.1:                                # %if.then
	movl	$1, -8(%rbp)
	jmp	.LBB3_3
.LBB3_2:                                # %if.end
	movl	$0, -8(%rbp)
.LBB3_3:                                # %return
	movl	-8(%rbp), %eax
	popq	%rbp
	retq
.Lfunc_end3:
	.size	collide_object, .Lfunc_end3-collide_object
	.cfi_endproc
                                        # -- End function
	.globl	collide_self            # -- Begin function collide_self
	.p2align	4, 0x90
	.type	collide_self,@function
collide_self:                           # @collide_self
	.cfi_startproc
# %bb.0:                                # %entry
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register %rbp
	movq	%rdi, -16(%rbp)
	movq	-16(%rbp), %rax
	addq	$12, %rax
	movq	-16(%rbp), %rcx
	movl	8(%rcx), %ecx
	subl	$1, %ecx
	movslq	%ecx, %rcx
	shlq	$3, %rcx
	addq	%rcx, %rax
	movq	%rax, -32(%rbp)
	movl	$0, -4(%rbp)
.LBB4_1:                                # %for.cond
                                        # =>This Inner Loop Header: Depth=1
	movl	-4(%rbp), %eax
	movq	-16(%rbp), %rcx
	movl	8(%rcx), %ecx
	subl	$1, %ecx
	cmpl	%ecx, %eax
	jge	.LBB4_7
# %bb.2:                                # %for.body
                                        #   in Loop: Header=BB4_1 Depth=1
	movq	-16(%rbp), %rax
	addq	$12, %rax
	movslq	-4(%rbp), %rcx
	shlq	$3, %rcx
	addq	%rcx, %rax
	movq	%rax, -24(%rbp)
	movq	-32(%rbp), %rax
	movl	(%rax), %eax
	movq	-24(%rbp), %rcx
	cmpl	(%rcx), %eax
	jne	.LBB4_5
# %bb.3:                                # %land.lhs.true
                                        #   in Loop: Header=BB4_1 Depth=1
	movq	-32(%rbp), %rax
	movl	4(%rax), %eax
	movq	-24(%rbp), %rcx
	cmpl	4(%rcx), %eax
	jne	.LBB4_5
# %bb.4:                                # %if.then
	movl	$1, -8(%rbp)
	jmp	.LBB4_8
.LBB4_5:                                # %if.end
                                        #   in Loop: Header=BB4_1 Depth=1
	jmp	.LBB4_6
.LBB4_6:                                # %for.inc
                                        #   in Loop: Header=BB4_1 Depth=1
	movl	-4(%rbp), %eax
	addl	$1, %eax
	movl	%eax, -4(%rbp)
	jmp	.LBB4_1
.LBB4_7:                                # %for.end
	movl	$0, -8(%rbp)
.LBB4_8:                                # %return
	movl	-8(%rbp), %eax
	popq	%rbp
	retq
.Lfunc_end4:
	.size	collide_self, .Lfunc_end4-collide_self
	.cfi_endproc
                                        # -- End function
	.globl	collision               # -- Begin function collision
	.p2align	4, 0x90
	.type	collision,@function
collision:                              # @collision
	.cfi_startproc
# %bb.0:                                # %entry
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register %rbp
	subq	$16, %rsp
	movq	%rdi, -8(%rbp)
	movq	%rsi, -16(%rbp)
	movq	-8(%rbp), %rdi
	callq	collide_walls
	movb	$1, %cl
	cmpl	$0, %eax
	jne	.LBB5_3
# %bb.1:                                # %lor.lhs.false
	movq	-8(%rbp), %rdi
	movq	-16(%rbp), %rsi
	movl	$42, %edx
	callq	collide_object
	movb	$1, %cl
	cmpl	$0, %eax
	jne	.LBB5_3
# %bb.2:                                # %lor.rhs
	movq	-8(%rbp), %rdi
	callq	collide_self
	cmpl	$0, %eax
	setne	%cl
.LBB5_3:                                # %lor.end
	andb	$1, %cl
	movzbl	%cl, %eax
	addq	$16, %rsp
	popq	%rbp
	retq
.Lfunc_end5:
	.size	collision, .Lfunc_end5-collision
	.cfi_endproc
                                        # -- End function
	.globl	eat_gold                # -- Begin function eat_gold
	.p2align	4, 0x90
	.type	eat_gold,@function
eat_gold:                               # @eat_gold
	.cfi_startproc
# %bb.0:                                # %entry
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register %rbp
	movq	%rdi, -16(%rbp)
	movq	%rsi, -8(%rbp)
	movq	-16(%rbp), %rax
	addq	$12, %rax
	movq	-16(%rbp), %rcx
	movl	8(%rcx), %ecx
	subl	$1, %ecx
	movslq	%ecx, %rcx
	shlq	$3, %rcx
	addq	%rcx, %rax
	movq	%rax, -24(%rbp)
	movq	-8(%rbp), %rax
	addq	$20, %rax
	movq	-24(%rbp), %rcx
	movl	(%rcx), %ecx
	subl	$1, %ecx
	movslq	%ecx, %rcx
	imulq	$78, %rcx, %rcx
	addq	%rcx, %rax
	movq	-24(%rbp), %rcx
	movl	4(%rcx), %ecx
	subl	$1, %ecx
	movslq	%ecx, %rcx
	movb	$32, (%rax,%rcx)
	movq	-8(%rbp), %rax
	movl	12(%rax), %ecx
	addl	$-1, %ecx
	movl	%ecx, 12(%rax)
	movq	-16(%rbp), %rax
	movl	8(%rax), %eax
	movq	-8(%rbp), %rcx
	imull	16(%rcx), %eax
	movq	-8(%rbp), %rcx
	addl	4(%rcx), %eax
	movl	%eax, 4(%rcx)
	movq	-16(%rbp), %rax
	movl	8(%rax), %ecx
	addl	$1, %ecx
	movl	%ecx, 8(%rax)
	movq	-8(%rbp), %rax
	movl	4(%rax), %eax
	movq	-8(%rbp), %rcx
	cmpl	8(%rcx), %eax
	jle	.LBB6_2
# %bb.1:                                # %if.then
	movq	-8(%rbp), %rax
	movl	4(%rax), %eax
	movq	-8(%rbp), %rcx
	movl	%eax, 8(%rcx)
.LBB6_2:                                # %if.end
	movq	-8(%rbp), %rax
	movl	12(%rax), %eax
	popq	%rbp
	retq
.Lfunc_end6:
	.size	eat_gold, .Lfunc_end6-eat_gold
	.cfi_endproc
                                        # -- End function
	.type	usec_delay,@object      # @usec_delay
	.data
	.globl	usec_delay
	.p2align	2
usec_delay:
	.long	200000                  # 0x30d40
	.size	usec_delay, 4

	.type	.L.str,@object          # @.str
	.section	.rodata.str1.1,"aMS",@progbits,1
.L.str:
	.asciz	"\033[2J\033[1;1H"
	.size	.L.str, 11

	.type	.L.str.2,@object        # @.str.2
.L.str.2:
	.asciz	"\033[%d;%dm"
	.size	.L.str.2, 9

	.type	.L.str.3,@object        # @.str.3
.L.str.3:
	.asciz	"\033[%d;%dH"
	.size	.L.str.3, 9

	.type	.L.str.10,@object       # @.str.10
.L.str.10:
	.asciz	"\033[%dm"
	.size	.L.str.10, 6

	.type	.L.str.11,@object       # @.str.11
.L.str.11:
	.asciz	"|"
	.size	.L.str.11, 2

	.type	.L.str.12,@object       # @.str.12
.L.str.12:
	.asciz	"%c"
	.size	.L.str.12, 3

	.type	.L.str.13,@object       # @.str.13
.L.str.13:
	.asciz	" "
	.size	.L.str.13, 2


	.ident	"clang version 6.0.0 (tags/RELEASE_600/final)"
	.section	".note.GNU-stack","",@progbits
