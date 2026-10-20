/*
 * The start of every atlOS user program: call main(), pass its return
 * value to exit (system call 0). The kernel starts the program here with
 * a 16 byte aligned stack and every register zero.
 */
.section .text.start, "ax"
.global _start
.type _start, @function
_start:
        andq $-16, %rsp         // the System V ABI wants rsp % 16 == 0 before a call
        call main
        movq %rax, %rdi         // exit(main())
        xorl %eax, %eax         // SysExit
        int $0x80
1:      jmp 1b                  // exit never returns
