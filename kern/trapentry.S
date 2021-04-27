/* See COPYRIGHT for copyright information. */

#include <inc/mmu.h>
#include <inc/memlayout.h>
#include <inc/trap.h>



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

	# Should never get here
