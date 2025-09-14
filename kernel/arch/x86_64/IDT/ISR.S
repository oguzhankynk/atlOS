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
 *  - sets DS/ES/FS/GS to kernel data selector (0x10) for flat model
 *  - aligns stack so that `call` obeys SystemV ABI: RSP % 16 == 8 before CALL
 *  - calls the handler with RDI = pointer to the stack frame
 *  - restores stack and registers, then iretq
 *
 * Note: r12 is used as temporary to hold pad value. r12 was saved on stack earlier,
 * so it is safe to clobber here and rely on the later `popq` to restore original r12.
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

    /* Call C handler */
    call \HANDLER

    /* Restore stack */
    add %r12, %rsp

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

    /* Pop int_no ve err_code */
    popq %rax      /* int_no */
    popq %rax      /* err_code */

    sti
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
