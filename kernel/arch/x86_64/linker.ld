OUTPUT_FORMAT(elf64-x86-64)
OUTPUT_ARCH(i386:x86-64)

ENTRY(_start)

SECTIONS
{
    . = KERNEL_PHYSICAL_START;

    /* Boot bölümleri */
    .boottext :
    {
        _boottext_start = .;
        *(.bootmultiboot)
        *(.boottext)
        _boottext_end = .;
    }

    .bootrodata :
    {
        _bootrodata_start = .;
        *(.bootrodata)
        _bootrodata_end = .;
    }

    .bootdata :
    {
        _bootdata_start = .;
        *(.bootdata)
        _bootdata_end = .;
    }

    .bootbss :
    {
        _bootbss_start = .;
        *(COMMON)
        *(.bootbss)
        _bootbss_end = .;
    }

    . = ALIGN(0x1000);
    _boot_physical_end = .;

    . += KERNEL_VIRTUAL_LOAD_OFFSET;
    kernel_virtual_start = .;
    kernel_physical_start = . - KERNEL_VIRTUAL_LOAD_OFFSET;

    /* Kernel text segmenti */
    .text : AT(_boot_physical_end)
    {
        _text_start = .;
        *(.multiboot)
        *(.text)
        _text_end = .;
    }

    . = ALIGN(0x1000);

    /* Read-only data */
    .rodata : AT((LOADADDR(.text) + SIZEOF(.text) + 0xFFF) & ~0xFFF)
    {
        _rodata_start = .;
        *(.rodata)
        _rodata_end = .;
    }

    . = ALIGN(0x1000);

    /* Data segmenti */
    .data : AT((LOADADDR(.rodata) + SIZEOF(.rodata) + 0xFFF) & ~0xFFF)
    {
        _data_start = .;
        *(.data)
        _data_end = .;
    }

    . = ALIGN(0x1000);

    
    .init_array : AT((LOADADDR(.data) + SIZEOF(.data) + 0xFFF) & ~0xFFF)
    {
        __init_array_start = .;
        KEEP(*(.init_array))
        KEEP(*(.init_array.*))
        __init_array_end = .;
    }

    
    .fini_array : AT((LOADADDR(.init_array) + SIZEOF(.init_array) + 0xFFF) & ~0xFFF)
    {
        __fini_array_start = .;
        KEEP(*(.fini_array))
        KEEP(*(.fini_array.*))
        __fini_array_end = .;
    } 

    .ctors : AT((LOADADDR(.fini_array) + SIZEOF(.fini_array) + 0xFFF) & ~0xFFF)
    {
        __ctors_start = .;
        KEEP(*(.ctors))
        KEEP(*(.ctors.*))
        __ctors_end = .;
    }

    .dtors : AT((LOADADDR(.ctors) + SIZEOF(.ctors) + 0xFFF) & ~0xFFF)
    {
        __dtors_start = .;
        KEEP(*(.dtors))
        KEEP(*(.dtors.*))
        __dtors_end = .;
    }

    . = ALIGN(0x1000);

    /* BSS segmenti */
    .bss : AT((LOADADDR(.dtors) + SIZEOF(.dtors) + 0xFFF) & ~0xFFF)
    {
        _bss_start = .;
        *(COMMON)
        *(.bss)
        _bss_end = .;
    }

    . = ALIGN(0x1000);
    kernel_virtual_end = .;
    kernel_physical_end = (LOADADDR(.bss) + SIZEOF(.bss) + 0xFFF) & ~0xFFF;
}
