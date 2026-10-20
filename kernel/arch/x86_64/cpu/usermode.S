/*
 * enterUserMode(rip, rsp): start running ring 3 code. Does not return; the
 * thread comes back to the kernel only through an interrupt, an exception
 * or a system call (int 0x80), on the TSS's rsp0 stack.
 *
 * iretq pops rip, cs, rflags, rsp, ss: push a frame that "returns" to ring 3.
 */

.section .text
.global enterUserMode
.type enterUserMode, @function
enterUserMode:
        pushq $0x23             // ss: user data segment (GDT entry 4) | RPL 3
        pushq %rsi              // rsp: the user stack
        pushq $0x202            // rflags: IF (interrupts on) and the always-1 bit
        pushq $0x1b             // cs: user code segment (GDT entry 3) | RPL 3
        pushq %rdi              // rip: the entry point

        // Nothing of the kernel's registers may leak into the program
        xorl %eax, %eax
        xorl %ebx, %ebx
        xorl %ecx, %ecx
        xorl %edx, %edx
        xorl %esi, %esi
        xorl %edi, %edi
        xorl %ebp, %ebp
        xorl %r8d, %r8d
        xorl %r9d, %r9d
        xorl %r10d, %r10d
        xorl %r11d, %r11d
        xorl %r12d, %r12d
        xorl %r13d, %r13d
        xorl %r14d, %r14d
        xorl %r15d, %r15d
        iretq
