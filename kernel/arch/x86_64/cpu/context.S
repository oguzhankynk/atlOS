.equ Regs.r15, 0
.equ Regs.r14, 8
.equ Regs.r13, 16
.equ Regs.r12, 24
.equ Regs.r11, 32
.equ Regs.r10, 40
.equ Regs.r9,  48
.equ Regs.r8,  56
.equ Regs.rbp, 64
.equ Regs.rdi, 72
.equ Regs.rsi, 80
.equ Regs.rdx, 88
.equ Regs.rcx, 96
.equ Regs.rbx, 104
.equ Regs.rax, 112
.equ Regs.rip, 120
.equ Regs.cs,  128
.equ Regs.rflags, 136
.equ Regs.rsp, 144
.equ Regs.ss,  152
.equ Regs.cr3, 160

.section .text
.global switchTask
switchTask:
    #void switchTask(Registers *from, Registers *to);

    # 1. SAVE CURRENT CONTEXT TO 'from' (RDI)
    # ---------------------------------------
    movq %r15, Regs.r15(%rdi)
    movq %r14, Regs.r14(%rdi)
    movq %r13, Regs.r13(%rdi)
    movq %r12, Regs.r12(%rdi)
    movq %r11, Regs.r11(%rdi)
    movq %r10, Regs.r10(%rdi)
    movq %r9,  Regs.r9(%rdi)
    movq %r8,  Regs.r8(%rdi)
    movq %rbp, Regs.rbp(%rdi)
    movq %rdi, Regs.rdi(%rdi) 
    movq %rsi, Regs.rsi(%rdi)
    movq %rdx, Regs.rdx(%rdi)
    movq %rcx, Regs.rcx(%rdi)
    movq %rbx, Regs.rbx(%rdi)
    movq %rax, Regs.rax(%rdi)

    # Save RIP (Return address is currently at the top of the stack)
    movq (%rsp), %rax
    movq %rax, Regs.rip(%rdi)

    # Save RSP (Add 8 because we want the RSP before the 'call' instruction pushed RIP)
    leaq 8(%rsp), %rax
    movq %rax, Regs.rsp(%rdi)

    # Save RFLAGS
    pushfq
    popq %rax
    movq %rax, Regs.rflags(%rdi)

    # Save CR3
    movq %cr3, %rax
    movq %rax, Regs.cr3(%rdi)

    # 2. LOAD NEW CONTEXT FROM 'to' (RSI)
    # ---------------------------------------
    # Switch Address Space (CR3)
    movq Regs.cr3(%rsi), %rax
    movq %rax, %cr3

    # Load General Purpose Registers
    movq Regs.r15(%rsi), %r15
    movq Regs.r14(%rsi), %r14
    movq Regs.r13(%rsi), %r13
    movq Regs.r12(%rsi), %r12
    movq Regs.r11(%rsi), %r11
    movq Regs.r10(%rsi), %r10
    movq Regs.r9(%rsi),  %r9
    movq Regs.r8(%rsi),  %r8
    movq Regs.rbp(%rsi), %rbp
    movq Regs.rdx(%rsi), %rdx
    movq Regs.rcx(%rsi), %rcx
    movq Regs.rbx(%rsi), %rbx
    # RDI and RSI will be loaded last

    # Load RFLAGS
    movq Regs.rflags(%rsi), %rax
    pushq %rax
    popfq

    # Switch Stack Pointer
    movq Regs.rsp(%rsi), %rsp

    # Prepare for the jump: Push the new RIP onto the new stack
    movq Regs.rip(%rsi), %rax
    pushq %rax

    # Finally load the last registers
    movq Regs.rdi(%rsi), %rdi # This is our 'this' pointer for threadStartPoint!
    movq Regs.rax(%rsi), %rax
    movq Regs.rsi(%rsi), %rsi

    # This 'ret' will pop the RIP we just pushed and jump to it
    ret
