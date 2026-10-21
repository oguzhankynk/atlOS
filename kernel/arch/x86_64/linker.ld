OUTPUT_FORMAT(elf64-x86-64)
OUTPUT_ARCH(i386:x86-64)

ENTRY(_start)

SECTIONS
{
    /* Kernel physical start */
    . = KERNEL_PHYSICAL_START;

    /* Boot sections */
    .boottext :
    {
        _boottext_start = .;
        *(.bootmultiboot)
        *(.boottext)
        _boottext_end = .;
        . = ALIGN(0x1000);
    }  

    .bootrodata :
    {
        _bootrodata_start = .;
        *(.bootrodata)
        _bootrodata_end = .;
        . = ALIGN(0x1000);
    }

    .bootdata :
    {
        _bootdata_start = .;
        *(.bootdata)
        _bootdata_end = .;
        . = ALIGN(0x1000);
    }

    .bootbss :
    {
        _bootbss_start = .;
        *(COMMON)
        *(.bootbss)
        _bootbss_end = .;
        . = ALIGN(0x1000);
    }

    _boot_physical_end = .; /* Boot segment end */

    /* Virtual mapping start */
    . = . + KERNEL_VIRTUAL_LOAD_OFFSET;
    kernel_virtual_start = .;
    kernel_physical_start = . - KERNEL_VIRTUAL_LOAD_OFFSET;

    /* Kernel text */
    .text : AT(_boot_physical_end)
    {
        _text_start = .;
        *(.multiboot)
        *(.text)
        *(.text.*)
        *(.init)     
        *(.fini)     
        *(.eh_frame)
        _text_end = .;
        . = ALIGN(0x200000);
    }

    /* Read-only data */
    .rodata : AT((LOADADDR(.text) + SIZEOF(.text) + (0x200000-1)) & ~(0x200000-1))
    {
         __ctors_start = .;
        *(.ctors)
        *(.ctors.*)
        *(.init_array)
        *(.init_array.*)
         __ctors_end = .;

         __dtors_start = .;
        *(.dtors)
        *(.dtors.*)
        *(.fini_array)
        *(.fini_array.*)
         __dtors_end = .;

        /* Exception table: (instruction that may fault, where to resume) pairs */
        __extable_start = .;
        KEEP(*(.extable))
        __extable_end = .;

        _rodata_start = .;
        *(.rodata)
        *(.rodata.*)
        _rodata_end = .;
        . = ALIGN(0x200000);
    }

    /* Data segment */
    .data : AT((LOADADDR(.rodata) + SIZEOF(.rodata) + (0x200000-1)) & ~(0x200000-1))
    {
        _data_start = .;
        *(.data)
        *(.data.*)
        _data_end = .;
        . = ALIGN(0x200000);
    }

    /* BSS */
    .bss : AT((LOADADDR(.data) + SIZEOF(.data) + (0x200000-1)) & ~(0x200000-1))
    {
        _bss_start = .;
        *(COMMON)
        *(.bss)
        _bss_end = .;
        . = ALIGN(0x200000);
    }

    kernel_virtual_end = .;
    kernel_physical_end = (LOADADDR(.bss) + SIZEOF(.bss) + (0x200000-1)) & ~(0x200000-1);

    /* --- Discard  sections --- */
    /DISCARD/ : {
        *(.comment)
        *(.symtab)
        *(.strtab)
        *(.shstrtab)
    }
}
