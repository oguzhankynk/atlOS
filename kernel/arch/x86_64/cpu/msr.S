/*
 * readMsr / writeMsr: rdmsr and wrmsr that may fault. Each faulting
 * instruction has an entry in the exception table (section .extable):
 * its address and the address to resume at. On a #GP there the fault
 * handler (Fault::fixup) changes the saved rip to the second one, so the
 * function returns false instead of the kernel panicking.
 */

.section .text

// bool readMsr(uint32_t msr, uint64_t* value): rdi = msr, rsi = value
.global readMsr
.type readMsr, @function
readMsr:
        movl %edi, %ecx
1:      rdmsr                   // edx:eax = MSR[ecx]
        shlq $32, %rdx
        orq %rdx, %rax
        movq %rax, (%rsi)
        movl $1, %eax
        ret
2:      xorl %eax, %eax         // resumed here after a #GP at 1
        ret

// bool writeMsr(uint32_t msr, uint64_t value): rdi = msr, rsi = value
.global writeMsr
.type writeMsr, @function
writeMsr:
        movl %edi, %ecx
        movl %esi, %eax         // low half
        movq %rsi, %rdx
        shrq $32, %rdx          // high half
3:      wrmsr
        movl $1, %eax
        ret
4:      xorl %eax, %eax
        ret

.section .extable, "a"
        .quad 1b, 2b
        .quad 3b, 4b
