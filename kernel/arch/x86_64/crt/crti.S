.section .init, "ax", @progbits
.align 4
.global _init
.type _init, @function
_init:
    pushq %rbp
    movq %rsp, %rbp
    andq $-16, %rsp

.section .fini, "ax", @progbits
.align 4
.global _fini
.type _fini, @function
_fini:
    pushq %rbp
    movq %rsp, %rbp
    andq $-16, %rsp