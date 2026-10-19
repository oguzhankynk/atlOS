.section .text
.extern int_handler    /* C/C++ exception handler symbol */

/* 
 * INT_DEFINITION NO HANDLER
 *  - NO: interrupt/vector number
 *  - HANDLER: label/name of C handler to call (expects pointer in RDI)
 *
 * This macro:
 *  - pushes error code (if exception does not supply one) and the vector number
 *  - saves general registers (rax..r15)
 *  - calls the handler with RDI = pointer to the stack frame
 *  - restores registers, then iretq
 */
.macro INT_DEFINITION NO HANDLER
    .global INT_\NO
    .align 4
INT_\NO:

    /* Eğer exception error code pushlamıyorsa dummy 0 */
    .if !(\NO == 8 || (\NO >= 10 && \NO <= 14) || \NO == 17 || \NO == 21)
        pushq $0          /* err_code */
    .endif

    /* Push exception/vector number */
    pushq $\NO            /* int_no */

    /* Push general-purpose registers (RAX..R15) */
    pushq %rax
    pushq %rbx
    pushq %rcx
    pushq %rdx
    pushq %rbp
    pushq %rsi
    pushq %rdi
    pushq %r8
    pushq %r9
    pushq %r10
    pushq %r11
    pushq %r12
    pushq %r13
    pushq %r14
    pushq %r15

    /* Pass pointer to current stack frame (RSP) in RDI */
    mov %rsp, %rdi

    /* Call C handler. The CPU aligns RSP to 16 before pushing the interrupt
     * frame; frame (5) + err_code + int_no + 15 registers = 22 qwords, so RSP
     * is already 16 byte aligned here and no padding is needed. */
    call \HANDLER

    /* Restore general-purpose registers */
    popq %r15
    popq %r14
    popq %r13
    popq %r12
    popq %r11
    popq %r10
    popq %r9
    popq %r8
    popq %rdi
    popq %rsi
    popq %rbp
    popq %rdx
    popq %rcx
    popq %rbx
    popq %rax

    /* Drop int_no and err_code without touching the restored registers */
    addq $16, %rsp

    /* iretq restores RFLAGS (and so IF) of the interrupted code */
    iretq
.endm


/* Generate handlers (same coverage as your original) */
.altmacro
    .set i, 0
    .rept 9
        INT_DEFINITION %i int_handler
        .set i, i + 1
    .endr

    .set i, 10
    .rept 5
        INT_DEFINITION %i int_handler
        .set i, i + 1
    .endr

    .set i, 16
    .rept 6
        INT_DEFINITION %i int_handler
        .set i, i + 1
    .endr

    .set i, 32
    .rept 224
        INT_DEFINITION %i int_handler
        .set i, i + 1
    .endr
