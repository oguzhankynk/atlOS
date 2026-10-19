
#include <arch/x86_64/boot/boot.inc>


.section .bootmultiboot, "a"
header_start:
#Source: https://www.gnu.org/software/grub/manual/multiboot2/multiboot.html
    .long MULTIBOOT2_HEADER_MAGIC
    .long MULTIBOOT_ARCHITECTURE_I386
    .long header_end - header_start
    .long -(MULTIBOOT2_HEADER_MAGIC + MULTIBOOT_ARCHITECTURE_I386 + header_end - header_start)

    // multiboot tags 

    .short MULTIBOOT_HEADER_TAG_END
    .short 0    // flags
    .long 8     // size
header_end:

.code32

.section .bootbss, "aw", @nobits
.comm pml4, PML4_SIZE, PML4_ALIGNMENT
.comm low_pdpt, PDPT_SIZE, PDPT_ALIGNMENT
.comm high_pdpt, PDPT_SIZE, PDPT_ALIGNMENT
.comm low_page_directory_table, PAGE_DIRECTORY_SIZE, PAGE_DIRECTORY_ALIGNMENT
.comm high_page_directory_table, PAGE_DIRECTORY_SIZE, PAGE_DIRECTORY_ALIGNMENT
.comm tmp_stack, KERNEL_BOOT_STACK_SIZE, KERNEL_BOOT_STACK_ALIGNMENT
.comm mbMagic, 8, 8
.comm mbInfo, 8, 8

.section .boottext, "ax"
.global _start
.type _start, @function
_start:

        //save multiboot info 
        movl %eax, mbMagic
        movl %ebx, mbInfo

        movl $tmp_stack + KERNEL_BOOT_STACK_SIZE, %esp

        movl $low_pdpt, %eax
        or $(MMU_PRESENT | MMU_WRITABLE), %eax
        movl %eax, pml4 + (PML4_ADDR_TO_ENTRY_INDEX(KERNEL_PHYSICAL_START) * PML4_ENTRY_SIZE)

        movl $high_pdpt, %eax
        or $(MMU_PRESENT | MMU_WRITABLE), %eax
        movl %eax, pml4 + (PML4_ADDR_TO_ENTRY_INDEX(KERNEL_VIRTUAL_START) * PML4_ENTRY_SIZE)

        movl $low_page_directory_table, %eax
        or $(MMU_PRESENT | MMU_WRITABLE), %eax
        movl %eax, low_pdpt + (PDPT_ADDR_TO_ENTRY_INDEX(KERNEL_PHYSICAL_START) * PDPT_ENTRY_SIZE)

        movl $high_page_directory_table, %eax
        or $(MMU_PRESENT | MMU_WRITABLE), %eax
        movl %eax, high_pdpt + (PDPT_ADDR_TO_ENTRY_INDEX(KERNEL_VIRTUAL_START) * PDPT_ENTRY_SIZE)

        mov $0, %ecx

        movl $kernel_physical_end, %esi
        shrl $0x15, %esi
        addl $1, %esi

page_directory_table_loop:
        movl $0x200000, %eax
        mul %ecx
        or $(MMU_PRESENT | MMU_WRITABLE | MMU_PDE_TWO_MB), %eax
        movl %eax, low_page_directory_table(, %ecx, PAGE_DIRECTORY_ENTRY_SIZE)
        movl %eax, high_page_directory_table(, %ecx, PAGE_DIRECTORY_ENTRY_SIZE)

        inc %ecx
        cmp %esi, %ecx
        jne page_directory_table_loop  // if not equal redo loop

        movl $pml4, %eax
        movl %eax, %cr3

        movl %cr4, %eax         // eax = CR4
        or $0x10, %eax          // PSE: Page Size Extension (bit 4)
        or $0x20, %eax          // PAE: Physical Address Extension (bit 5)
        movl %eax, %cr4         // Write back to CR4

        movl $MSR_EFER, %ecx
        rdmsr
        or $MSR_EFER_LME, %eax
        wrmsr

        movl %cr0, %eax             // Read current CR0 register value
        or $0x1, %eax               // PE (Protected Mode Enable) = 1
        or $0x10, %eax              // ET (Extension Type) = 1 (FPU present)
        or $0x80000000, %eax        // PG (Paging) = 1 to enable paging
        movl %eax, %cr0             // Write modified value back to CR0

        .extern gdt_ptr
        lgdt gdt_ptr

        ljmp $0x08, $_start64 // Far jump to 64-bit kernel code segment (CS=0x08), which is the 1st entry in the GDT, and RIP=_start64

        cli
        hlt

.code64

.global _start64
.type _start64, @function
_start64:
        // Setup segment selectors
        movw $0, %ax
        movw %ax, %ds
        movw %ax, %es
        movw %ax, %fs
        movw %ax, %gs
        movw %ax, %ss

        movq mbInfo(%rip), %rdi   
        movl mbMagic(%rip), %esi  

        
        call kernelMain

        cli
        hlt
1:
        jmp 1b