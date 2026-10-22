/*
 * The start of every atlOS user program. The kernel starts it here with the
 * System V stack: rsp points at argc, argv[] follows it (see
 * kernel/proc/README.md). _start calls main(argc, argv) and passes its
 * return value to exit (system call 0). Every other register is zero.
 */
.section .text.start, "ax"
.global _start
.type _start, @function
_start:
        movq (%rsp), %rdi       // argc
        leaq 8(%rsp), %rsi      // argv: right above argc
        andq $-16, %rsp         // the System V ABI wants rsp % 16 == 0 before a call
        call main
        movq %rax, %rdi         // exit(main(argc, argv))
        xorl %eax, %eax         // SysExit
        int $0x80
1:      jmp 1b                  // exit never returns
