	.text
	.file	"snake.bc"
	.globl	sigsetup                # -- Begin function sigsetup
	.p2align	4, 0x90
	.type	sigsetup,@function
sigsetup:                               # @sigsetup
	.cfi_startproc
# %bb.0:                                # %entry
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register %rbp
	subq	$176, %rsp
	leaq	-168(%rbp), %rax
	movl	%edi, -4(%rbp)
	movq	%rsi, -16(%rbp)
	addq	$8, %rax
	movq	%rax, %rdi
	callq	sigemptyset
	movl	$14, %eax
	movl	$0, -32(%rbp)
	movq	-16(%rbp), %rcx
	movq	%rcx, -168(%rbp)
	cmpl	-4(%rbp), %eax
	jne	.LBB0_2
# %bb.1:                                # %if.then
	movl	-32(%rbp), %eax
	orl	$536870912, %eax        # imm = 0x20000000
	movl	%eax, -32(%rbp)
	jmp	.LBB0_3
.LBB0_2:                                # %if.else
	movl	-32(%rbp), %eax
	orl	$268435456, %eax        # imm = 0x10000000
	movl	%eax, -32(%rbp)
.LBB0_3:                                # %if.end
	leaq	-168(%rbp), %rsi
	movl	-4(%rbp), %edi
	xorl	%edx, %edx
	callq	sigaction
	addq	$176, %rsp
	popq	%rbp
	retq
.Lfunc_end0:
	.size	sigsetup, .Lfunc_end0-sigsetup
	.cfi_endproc
                                        # -- End function
	.globl	sig_handler             # -- Begin function sig_handler
	.p2align	4, 0x90
	.type	sig_handler,@function
sig_handler:                            # @sig_handler
	.cfi_startproc
# %bb.0:                                # %entry
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register %rbp
	subq	$16, %rsp
	movabsq	$.L.str, %rax
	movl	%edi, -4(%rbp)
	movq	%rax, %rdi
	callq	puts
	movabsq	$.L.str.1, %rdi
	callq	system
	andl	$65280, %eax            # imm = 0xFF00
	sarl	$8, %eax
	movl	%eax, %edi
	callq	exit
.Lfunc_end1:
	.size	sig_handler, .Lfunc_end1-sig_handler
	.cfi_endproc
                                        # -- End function
	.globl	alarm_handler           # -- Begin function alarm_handler
	.p2align	4, 0x90
	.type	alarm_handler,@function
alarm_handler:                          # @alarm_handler
	.cfi_startproc
# %bb.0:                                # %entry
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register %rbp
	subq	$16, %rsp
	movl	%edi, -4(%rbp)
	cmpl	$0, -4(%rbp)
	jne	.LBB2_2
# %bb.1:                                # %if.then
	movabsq	$alarm_handler, %rsi
	movl	$14, %edi
	callq	sigsetup
.LBB2_2:                                # %if.end
	movabsq	$alarm_handler.val, %rsi
	movq	$0, alarm_handler.val+16
	movl	usec_delay, %eax
	movq	%rax, alarm_handler.val+24
	xorl	%edi, %edi
	xorl	%edx, %edx
	callq	setitimer
	addq	$16, %rsp
	popq	%rbp
	retq
.Lfunc_end2:
	.size	alarm_handler, .Lfunc_end2-alarm_handler
	.cfi_endproc
                                        # -- End function
	.globl	show_score              # -- Begin function show_score
	.p2align	4, 0x90
	.type	show_score,@function
show_score:                             # @show_score
	.cfi_startproc
# %bb.0:                                # %entry
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register %rbp
	subq	$16, %rsp
	movabsq	$.L.str.2, %rax
	movq	%rdi, -8(%rbp)
	movq	%rax, %rdi
	movl	$1, %esi
	movl	$36, %edx
	movb	$0, %al
	callq	printf
	movabsq	$.L.str.3, %rdi
	movl	$24, %esi
	movl	$3, %edx
	movb	$0, %al
	callq	printf
	movabsq	$.L.str.4, %rdi
	movq	-8(%rbp), %rax
	movl	(%rax), %esi
	movb	$0, %al
	callq	printf
	movabsq	$.L.str.2, %rdi
	movl	$1, %esi
	movl	$33, %edx
	movb	$0, %al
	callq	printf
	movabsq	$.L.str.3, %rdi
	movl	$24, %esi
	movl	$21, %edx
	movb	$0, %al
	callq	printf
	movabsq	$.L.str.5, %rdi
	movq	-8(%rbp), %rax
	movl	12(%rax), %esi
	movb	$0, %al
	callq	printf
	movabsq	$.L.str.2, %rdi
	movl	$1, %esi
	movl	$32, %edx
	movb	$0, %al
	callq	printf
	movabsq	$.L.str.3, %rdi
	movl	$24, %esi
	movl	$43, %edx
	movb	$0, %al
	callq	printf
	movabsq	$.L.str.6, %rdi
	movq	-8(%rbp), %rax
	movl	4(%rax), %esi
	movb	$0, %al
	callq	printf
	movabsq	$.L.str.2, %rdi
	movl	$1, %esi
	movl	$35, %edx
	movb	$0, %al
	callq	printf
	movabsq	$.L.str.3, %rdi
	movl	$24, %esi
	movl	$61, %edx
	movb	$0, %al
	callq	printf
	movabsq	$.L.str.7, %rdi
	movq	-8(%rbp), %rax
	movl	8(%rax), %esi
	movb	$0, %al
	callq	printf
	addq	$16, %rsp
	popq	%rbp
	retq
.Lfunc_end3:
	.size	show_score, .Lfunc_end3-show_score
	.cfi_endproc
                                        # -- End function
	.globl	draw_line               # -- Begin function draw_line
	.p2align	4, 0x90
	.type	draw_line,@function
draw_line:                              # @draw_line
	.cfi_startproc
# %bb.0:                                # %entry
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register %rbp
	subq	$16, %rsp
	movabsq	$.L.str.3, %rax
	movl	%edi, -12(%rbp)
	movl	%esi, -8(%rbp)
	movl	-8(%rbp), %esi
	movl	-12(%rbp), %edx
	movq	%rax, %rdi
	movb	$0, %al
	callq	printf
	movabsq	$.L.str.2, %rdi
	movl	$1, %esi
	movl	$44, %edx
	movb	$0, %al
	callq	printf
	movabsq	$.L.str.2, %rdi
	movl	$1, %esi
	movl	$34, %edx
	movb	$0, %al
	callq	printf
	movl	$0, -4(%rbp)
.LBB4_1:                                # %for.cond
                                        # =>This Inner Loop Header: Depth=1
	cmpl	$80, -4(%rbp)
	jge	.LBB4_8
# %bb.2:                                # %for.body
                                        #   in Loop: Header=BB4_1 Depth=1
	cmpl	$0, -4(%rbp)
	je	.LBB4_4
# %bb.3:                                # %lor.lhs.false
                                        #   in Loop: Header=BB4_1 Depth=1
	cmpl	$79, -4(%rbp)
	jne	.LBB4_5
.LBB4_4:                                # %if.then
                                        #   in Loop: Header=BB4_1 Depth=1
	movabsq	$.L.str.8, %rdi
	movb	$0, %al
	callq	printf
	jmp	.LBB4_6
.LBB4_5:                                # %if.else
                                        #   in Loop: Header=BB4_1 Depth=1
	movabsq	$.L.str.9, %rdi
	movb	$0, %al
	callq	printf
.LBB4_6:                                # %if.end
                                        #   in Loop: Header=BB4_1 Depth=1
	jmp	.LBB4_7
.LBB4_7:                                # %for.inc
                                        #   in Loop: Header=BB4_1 Depth=1
	movl	-4(%rbp), %eax
	addl	$1, %eax
	movl	%eax, -4(%rbp)
	jmp	.LBB4_1
.LBB4_8:                                # %for.end
	movabsq	$.L.str.10, %rdi
	xorl	%esi, %esi
	movb	$0, %al
	callq	printf
	addq	$16, %rsp
	popq	%rbp
	retq
.Lfunc_end4:
	.size	draw_line, .Lfunc_end4-draw_line
	.cfi_endproc
                                        # -- End function
	.globl	main                    # -- Begin function main
	.p2align	4, 0x90
	.type	main,@function
main:                                   # @main
	.cfi_startproc
# %bb.0:                                # %entry
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register %rbp
	subq	$2576, %rsp             # imm = 0xA10
	movabsq	$.L.str.14, %rdi
	movl	$0, -8(%rbp)
	movl	.Lmain.keys, %eax
	movl	%eax, -15(%rbp)
	movw	.Lmain.keys+4, %ax
	movw	%ax, -11(%rbp)
	movb	.Lmain.keys+6, %al
	movb	%al, -9(%rbp)
	callq	system
	andl	$65280, %eax            # imm = 0xFF00
	sarl	$8, %eax
	cmpl	$0, %eax
	je	.LBB5_2
# %bb.1:                                # %if.then
	movabsq	$.L.str.15, %rsi
	movq	stderr, %rdi
	movb	$0, %al
	callq	fprintf
	movl	$1, -8(%rbp)
	jmp	.LBB5_22
.LBB5_2:                                # %if.end
	xorl	%edi, %edi
	callq	alarm_handler
	movabsq	$sig_handler, %rsi
	movl	$2, %edi
	callq	sigsetup
	movabsq	$sig_handler, %rsi
	movl	$1, %edi
	callq	sigsetup
	movabsq	$sig_handler, %rsi
	movl	$15, %edi
	callq	sigsetup
.LBB5_3:                                # %do.body
                                        # =>This Loop Header: Depth=1
                                        #     Child Loop BB5_4 Depth 2
                                        #     Child Loop BB5_15 Depth 2
	leaq	-2568(%rbp), %rdi
	leaq	-832(%rbp), %rsi
	movl	$1, %edx
	callq	setup_level
.LBB5_4:                                # %do.body5
                                        #   Parent Loop BB5_3 Depth=1
                                        # =>  This Inner Loop Header: Depth=2
	callq	getchar
	leaq	-832(%rbp), %rdi
	leaq	-15(%rbp), %rsi
	movb	%al, -1(%rbp)
	movsbl	-1(%rbp), %edx
	callq	move
	movabsq	$.L.str.3, %rdi
	movl	$1, %esi
	movl	$1, %edx
	movb	$0, %al
	callq	printf
	leaq	-832(%rbp), %rdi
	leaq	-2568(%rbp), %rsi
	callq	collision
	cmpl	$0, %eax
	je	.LBB5_6
# %bb.5:                                # %if.then10
                                        #   in Loop: Header=BB5_3 Depth=1
	movb	-9(%rbp), %al
	movb	%al, -1(%rbp)
	jmp	.LBB5_14
.LBB5_6:                                # %if.else
                                        #   in Loop: Header=BB5_4 Depth=2
	leaq	-832(%rbp), %rdi
	leaq	-2568(%rbp), %rsi
	movl	$36, %edx
	callq	collide_object
	cmpl	$0, %eax
	je	.LBB5_10
# %bb.7:                                # %if.then13
                                        #   in Loop: Header=BB5_4 Depth=2
	leaq	-832(%rbp), %rdi
	leaq	-2568(%rbp), %rsi
	callq	eat_gold
	cmpl	$0, %eax
	jne	.LBB5_9
# %bb.8:                                # %if.then16
                                        #   in Loop: Header=BB5_4 Depth=2
	leaq	-2568(%rbp), %rdi
	leaq	-832(%rbp), %rsi
	xorl	%edx, %edx
	callq	setup_level
.LBB5_9:                                # %if.end17
                                        #   in Loop: Header=BB5_4 Depth=2
	leaq	-2568(%rbp), %rdi
	callq	show_score
.LBB5_10:                               # %if.end18
                                        #   in Loop: Header=BB5_4 Depth=2
	jmp	.LBB5_11
.LBB5_11:                               # %if.end19
                                        #   in Loop: Header=BB5_4 Depth=2
	jmp	.LBB5_12
.LBB5_12:                               # %do.cond
                                        #   in Loop: Header=BB5_4 Depth=2
	movsbl	-1(%rbp), %eax
	movsbl	-9(%rbp), %ecx
	cmpl	%ecx, %eax
	jne	.LBB5_4
# %bb.13:                               # %do.end.loopexit
                                        #   in Loop: Header=BB5_3 Depth=1
	jmp	.LBB5_14
.LBB5_14:                               # %do.end
                                        #   in Loop: Header=BB5_3 Depth=1
	leaq	-2568(%rbp), %rdi
	callq	show_score
	movabsq	$.L.str.3, %rdi
	movl	$6, %esi
	movl	$32, %edx
	movb	$0, %al
	callq	printf
	movabsq	$.L.str.2, %rdi
	movl	$1, %esi
	movl	$31, %edx
	movb	$0, %al
	callq	printf
	movabsq	$.L.str.16, %rdi
	movb	$0, %al
	callq	printf
	movabsq	$.L.str.3, %rdi
	movl	$9, %esi
	movl	$32, %edx
	movb	$0, %al
	callq	printf
	movabsq	$.L.str.2, %rdi
	movl	$1, %esi
	movl	$33, %edx
	movb	$0, %al
	callq	printf
	movabsq	$.L.str.17, %rdi
	movb	$0, %al
	callq	printf
.LBB5_15:                               # %do.body30
                                        #   Parent Loop BB5_3 Depth=1
                                        # =>  This Inner Loop Header: Depth=2
	callq	getchar
	movb	%al, -1(%rbp)
# %bb.16:                               # %do.cond33
                                        #   in Loop: Header=BB5_15 Depth=2
	xorl	%eax, %eax
	movsbl	-1(%rbp), %ecx
	cmpl	$121, %ecx
	je	.LBB5_18
# %bb.17:                               # %land.rhs
                                        #   in Loop: Header=BB5_15 Depth=2
	movsbl	-1(%rbp), %eax
	cmpl	$110, %eax
	setne	%al
.LBB5_18:                               # %land.end
                                        #   in Loop: Header=BB5_15 Depth=2
	testb	$1, %al
	jne	.LBB5_15
# %bb.19:                               # %do.end40
                                        #   in Loop: Header=BB5_3 Depth=1
	jmp	.LBB5_20
.LBB5_20:                               # %do.cond41
                                        #   in Loop: Header=BB5_3 Depth=1
	movsbl	-1(%rbp), %eax
	cmpl	$121, %eax
	je	.LBB5_3
# %bb.21:                               # %do.end45
	movabsq	$.L.str, %rdi
	callq	puts
	movabsq	$.L.str.1, %rdi
	callq	system
	andl	$65280, %eax            # imm = 0xFF00
	sarl	$8, %eax
	movl	%eax, -8(%rbp)
.LBB5_22:                               # %return
	movl	-8(%rbp), %eax
	addq	$2576, %rsp             # imm = 0xA10
	popq	%rbp
	retq
.Lfunc_end5:
	.size	main, .Lfunc_end5-main
	.cfi_endproc
                                        # -- End function
	.globl	set_app_usec_delay      # -- Begin function set_app_usec_delay
	.p2align	4, 0x90
	.type	set_app_usec_delay,@function
set_app_usec_delay:                     # @set_app_usec_delay
	.cfi_startproc
# %bb.0:                                # %entry
	movl	%edi, -4(%rsp)
	movl	%edi, usec_delay(%rip)
	retq
.Lfunc_end6:
	.size	set_app_usec_delay, .Lfunc_end6-set_app_usec_delay
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

	.type	.L.str.1,@object        # @.str.1
.L.str.1:
	.asciz	"stty sane"
	.size	.L.str.1, 10

	.type	alarm_handler.val,@object # @alarm_handler.val
	.local	alarm_handler.val
	.comm	alarm_handler.val,32,8
	.type	.L.str.2,@object        # @.str.2
.L.str.2:
	.asciz	"\033[%d;%dm"
	.size	.L.str.2, 9

	.type	.L.str.3,@object        # @.str.3
.L.str.3:
	.asciz	"\033[%d;%dH"
	.size	.L.str.3, 9

	.type	.L.str.4,@object        # @.str.4
.L.str.4:
	.asciz	"Level: %05d"
	.size	.L.str.4, 12

	.type	.L.str.5,@object        # @.str.5
.L.str.5:
	.asciz	"Gold Left: %05d"
	.size	.L.str.5, 16

	.type	.L.str.6,@object        # @.str.6
.L.str.6:
	.asciz	"Score: %05d"
	.size	.L.str.6, 12

	.type	.L.str.7,@object        # @.str.7
.L.str.7:
	.asciz	"High Score: %05d"
	.size	.L.str.7, 17

	.type	.L.str.8,@object        # @.str.8
.L.str.8:
	.asciz	"+"
	.size	.L.str.8, 2

	.type	.L.str.9,@object        # @.str.9
.L.str.9:
	.asciz	"-"
	.size	.L.str.9, 2

	.type	.L.str.10,@object       # @.str.10
.L.str.10:
	.asciz	"\033[%dm"
	.size	.L.str.10, 6

	.type	.Lmain.keys,@object     # @main.keys
	.section	.rodata,"a",@progbits
.Lmain.keys:
	.ascii	"opazfjq"
	.size	.Lmain.keys, 7

	.type	.L.str.14,@object       # @.str.14
	.section	.rodata.str1.1,"aMS",@progbits,1
.L.str.14:
	.asciz	"stty cbreak -echo stop u"
	.size	.L.str.14, 25

	.type	.L.str.15,@object       # @.str.15
.L.str.15:
	.asciz	"Failed setting up the screen, is 'stty' missing?\n"
	.size	.L.str.15, 50

	.type	.L.str.16,@object       # @.str.16
.L.str.16:
	.asciz	"-G A M E  O V E R-"
	.size	.L.str.16, 19

	.type	.L.str.17,@object       # @.str.17
.L.str.17:
	.asciz	"Another Game (y/n)? "
	.size	.L.str.17, 21


	.ident	"clang version 6.0.0 (tags/RELEASE_600/final)"
	.section	".note.GNU-stack","",@progbits
