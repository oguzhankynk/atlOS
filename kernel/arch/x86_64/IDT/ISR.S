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

    /* If the exception does NOT push an error code, push a dummy 0 */
    .if !(\NO == 8 || (\NO >= 10 && \NO <= 14) || \NO == 17 || \NO == 21)
        pushq $0
    .endif

    /* Push exception/vector number */
    pushq $\NO

    /* Save general-purpose registers (caller-saved + callee-saved) */
    pushq %rax
    pushq %rbx
    pushq %rcx
    pushq %rdx
    pushq %rsi
    pushq %rdi
    pushq %rbp
    pushq %r8
    pushq %r9
    pushq %r10
    pushq %r11
    pushq %r12
    pushq %r13
    pushq %r14
    pushq %r15

    /* Load kernel data selector into segment registers (flat model) */
    mov $0x10, %ax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs

    /* ---- Stack alignment before calling C handler ----
       Compute pad = ((RSP & 0xF) - 8) & 0xF
       Then do: sub pad, %rsp  so that RSP % 16 == 8 before CALL
       Save pad in r12 (which was pushed above), so we can restore after call.
    */
    mov %rsp, %rax
    and $0xF, %rax            /* rax = rsp & 0xF */
    sub $8, %rax              /* rax = (rsp & 0xF) - 8 */
    and $0xF, %rax            /* rax = ((rsp & 0xF) - 8) & 0xF  -> pad */
    mov %rax, %r12            /* r12 = pad (r12 previously pushed, so safe) */
    sub %r12, %rsp            /* adjust RSP by pad */

    /* Pass pointer to current stack-frame (RSP) in RDI (first arg) */
    mov %rsp, %rdi

    /* Call the C handler */
    call \HANDLER

    /* restore stack adjustment: add pad back */
    add %r12, %rsp

    /* Restore segment registers (optional for flat model; harmless) */
    mov $0x10, %ax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs

    /* Restore general-purpose registers in reverse order of push */
    popq %r15
    popq %r14
    popq %r13
    popq %r12
    popq %r11
    popq %r10
    popq %r9
    popq %r8
    popq %rbp
    popq %rdi
    popq %rsi
    popq %rdx
    popq %rcx
    popq %rbx
    popq %rax

    /* Pop exception number and error code (if any) */
    popq %rax    /* exception number */
    popq %rax    /* error code */

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
