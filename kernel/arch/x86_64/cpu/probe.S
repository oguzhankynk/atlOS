/*
 * readQword(address, value): a load that may fault, with an exception
 * table entry like the ones in msr.S.
 */

.section .text

// bool readQword(const void* address, uint64_t* value): rdi = address, rsi = value
.global readQword
.type readQword, @function
readQword:
1:      movq (%rdi), %rax       // may page fault or #GP
        movq %rax, (%rsi)
        movl $1, %eax
        ret
2:      xorl %eax, %eax         // resumed here after a fault at 1
        ret

.section .extable, "a"
        .quad 1b, 2b
