/* See COPYRIGHT for copyright information. */

#include <inc/mmu.h>
#include <inc/memlayout.h>
#include <inc/trap.h>

#include <kern/picirq.h>


###################################################################
# exceptions/interrupts
###################################################################

/* TRAPHANDLER defines a globally-visible function for handling a trap.
 * It pushes a trap number onto the stack, then jumps to _alltraps.
 * Use TRAPHANDLER for traps where the CPU automatically pushes an error code.
 *
 * You shouldn't call a TRAPHANDLER function from C, but you may
 * need to _declare_ one in C (for instance, to get a function pointer
 * during IDT setup).  You can declare the function with
 *   void NAME();
 * where NAME is the argument passed to TRAPHANDLER.
 */
#define TRAPHANDLER(name, num)						\
	.text;								\
	.globl name;		/* define global symbol for 'name' */	\
	.type name, @function;	/* symbol type is function */		\
	.align 2;		/* align function definition */		\
	name:			/* function starts here */		\
	pushl $(num);							\
	jmp _alltraps;							\
									\
	.data;								\
	.long name		/* save address into the table */	

/* Use TRAPHANDLER_NOEC for traps where the CPU doesn't push an error code.
 * It pushes a 0 in place of the error code, so the trap frame has the same
 * format in either case.
 */
#define TRAPHANDLER_NOEC(name, num)					\
	.text;								\
	.globl name;							\
	.type name, @function;						\
	.align 2;							\
	name:								\
	pushl $0;							\
	pushl $(num);							\
	jmp _alltraps;							\
									\
	.data;								\
	.long name

.text

/*
 * Lab 3: Your code here for generating entry points for the different traps.
 */

.data

.globl trap_handlers
.align 4
trap_handlers:

TRAPHANDLER_NOEC(traphandler_divide, T_DIVIDE)
TRAPHANDLER_NOEC(traphandler_debug, T_DEBUG)
TRAPHANDLER_NOEC(traphandler_nmi, T_NMI)
TRAPHANDLER_NOEC(traphandler_brkpt, T_BRKPT)
TRAPHANDLER_NOEC(traphandler_oflow, T_OFLOW)
TRAPHANDLER_NOEC(traphandler_bound, T_BOUND)
TRAPHANDLER_NOEC(traphandler_illop, T_ILLOP)
TRAPHANDLER_NOEC(traphandler_device, T_DEVICE)
TRAPHANDLER(traphandler_dblflt, T_DBLFLT)
TRAPHANDLER_NOEC(traphandler_coproc, T_COPROC)
TRAPHANDLER(traphandler_tss, T_TSS)
TRAPHANDLER(traphandler_segnp, T_SEGNP)
TRAPHANDLER(traphandler_stack, T_STACK)
TRAPHANDLER(traphandler_gpflt, T_GPFLT)
TRAPHANDLER(traphandler_pgflt, T_PGFLT)
TRAPHANDLER(traphandler_res, T_RES)
TRAPHANDLER_NOEC(traphandler_fperr, T_FPERR)
TRAPHANDLER(traphandler_align, T_ALIGN)
TRAPHANDLER_NOEC(traphandler_mchk, T_MCHK)
TRAPHANDLER_NOEC(traphandler_simderr, T_SIMDERR)
TRAPHANDLER_NOEC(traphandler_20, 20)
TRAPHANDLER_NOEC(traphandler_21, 21)
TRAPHANDLER_NOEC(traphandler_22, 22)
TRAPHANDLER_NOEC(traphandler_23, 23)
TRAPHANDLER_NOEC(traphandler_24, 24)
TRAPHANDLER_NOEC(traphandler_25, 25)
TRAPHANDLER_NOEC(traphandler_26, 26)
TRAPHANDLER_NOEC(traphandler_27, 27)
TRAPHANDLER_NOEC(traphandler_28, 28)
TRAPHANDLER_NOEC(traphandler_29, 29)
TRAPHANDLER_NOEC(traphandler_30, 30)
TRAPHANDLER_NOEC(traphandler_31, 31)
TRAPHANDLER_NOEC(traphandler_timer, IRQ_OFFSET + IRQ_TIMER)
TRAPHANDLER_NOEC(traphandler_kbd, IRQ_OFFSET + IRQ_KBD)
TRAPHANDLER_NOEC(traphandler_irq2, IRQ_OFFSET + 2)
TRAPHANDLER_NOEC(traphandler_irq3, IRQ_OFFSET + 3)
TRAPHANDLER_NOEC(traphandler_serial, IRQ_OFFSET + IRQ_SERIAL)
TRAPHANDLER_NOEC(traphandler_irq5, IRQ_OFFSET + 5)
TRAPHANDLER_NOEC(traphandler_irq6, IRQ_OFFSET + 6)
TRAPHANDLER_NOEC(traphandler_spurious, IRQ_OFFSET + IRQ_SPURIOUS)
TRAPHANDLER_NOEC(traphandler_irq8, IRQ_OFFSET + 8)
TRAPHANDLER_NOEC(traphandler_irq9, IRQ_OFFSET + 9)
TRAPHANDLER_NOEC(traphandler_irq10, IRQ_OFFSET + 10)
TRAPHANDLER_NOEC(traphandler_irq11, IRQ_OFFSET + 11)
TRAPHANDLER_NOEC(traphandler_irq12, IRQ_OFFSET + 12)
TRAPHANDLER_NOEC(traphandler_irq13, IRQ_OFFSET + 13)
TRAPHANDLER_NOEC(traphandler_ide, IRQ_OFFSET + IRQ_IDE)
TRAPHANDLER_NOEC(traphandler_irq15, IRQ_OFFSET + 15)
TRAPHANDLER_NOEC(traphandler_syscall, T_SYSCALL)
TRAPHANDLER_NOEC(traphandler_ird17, IRQ_OFFSET + 17)
TRAPHANDLER_NOEC(traphandler_ird18, IRQ_OFFSET + 18)
TRAPHANDLER_NOEC(traphandler_ird19, IRQ_OFFSET + IRQ_ERROR)

/*
 * Lab 3: Your code here for _alltraps
 */

.text

.globl _alltraps
_alltraps:
	# Finish building a struct Trapframe
	pushl %ds
	pushl %es
	pushal

        # Load kernel data segments
	movw $GD_KD, %ax
	movw %ax, %ds
	movw %ax, %es

	# Call trap(tf)
	pushl %esp
	call trap
	addl $0x4, %esp

	# Restore registers
	popal
	popl %es
	popl %ds

	# Remove tf_trapno and tf_err
	addl $0x8, %esp

	iret


.globl sysenter_handler
sysenter_handler:
	# Save user data segments
	pushl	%ds
	pushl	%es

	# Push the arguments onto the stack
	pushl	$0
	pushl	%edi
	pushl	%ebx
	pushl	%ecx
	pushl	%edx
	pushl	%eax

	# Load kernel data segments
	movw	$GD_KD, %ax
	movw	%ax, %ds
	movw	%ax, %es

	# Call syscall(num, a1, a2, a3, a4, 0)
	call	syscall

	# Remove the arguments from the stack
	addl	$0x18, %esp

	# Restore user data segments
	popl	%es
	popl	%ds
	
	movl	%esi, %edx	# User-mode return address -> %edx
	movl	%ebp, %ecx	# User-mode stack pointer -> ecx

	sysexit
