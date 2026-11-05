/*
 * The start of every atlOS user program. The kernel starts it here with the
 * System V stack: rsp points at argc, argv[], envp[] and the auxiliary
 * vector follow it (see kernel/proc/README.md). _start hands that stack to
 * atlos_start (start.cpp), which sets up the heap, calls main(argc, argv)
 * and passes its return value to exit. Every other register is zero.
 */
.section .text.start, "ax"
.global _start
.type _start, @function
_start:
        movq %rsp, %rdi         // the start stack: argc, argv[], ...
        andq $-16, %rsp         // the System V ABI wants rsp % 16 == 0 before a call
        call atlos_start
1:      jmp 1b                  // atlos_start never returns
