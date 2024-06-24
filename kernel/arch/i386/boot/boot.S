# Declare constants for the multiboot header.
.set ALIGN,    1<<0             # align loaded modules on page boundaries
.set MEMINFO,  1<<1             # provide memory map
.set FLAGS,    ALIGN | MEMINFO  # this is the Multiboot 'flag' field
.set MAGIC,    0x1BADB002       # 'magic number' lets bootloader find the header
.set CHECKSUM, -(MAGIC + FLAGS) # checksum of above, to prove we are multiboot

# Declare a header as in the Multiboot Standard.
.section .multiboot, "ax", @progbits
.align 4
.long MAGIC
.long FLAGS
.long CHECKSUM

# Reserve a stack for the initial thread.
.section .bss
.align 16
stack_bottom:
.skip 16384 # 16 KiB
stack_top:


.section .data
.align 4096
.global initial_page_dir
initial_page_dir:
    .long 0b10000011
    .rept 768-1
        .long 0
    .endr

    .long (0 << 22) | 0b10000011
    .long (1 << 22) | 0b10000011
    .long (2 << 22) | 0b10000011
    .long (3 << 22) | 0b10000011
    .rept 256-4
        .long 0
    .endr

.section .boot, "ax", @progbits
.global _start
_start:
    movl $initial_page_dir - 0xC0000000, %ecx
    movl %ecx, %cr3

    movl %cr4, %ecx
    orl $0x10, %ecx
    movl %ecx, %cr4

    movl %cr0, %ecx
    orl $0x80000000, %ecx
    movl %ecx, %cr0

    jmp higher_half

.section .text
.global higher_half
higher_half:
    movl $stack_top, %esp
    push %ebx
    push %eax
    xorl %ebp, %ebp
    call kernel_main

halt:
    hlt
    jmp halt

