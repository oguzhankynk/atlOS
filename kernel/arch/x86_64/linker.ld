OUTPUT_FORMAT( elf64-x86-64 )
OUTPUT_ARCH( i386:x86-64 )


ENTRY (_start)

SECTIONS
{
    . = 0x0000000000400000;

    .boottext :
    {
        *(.bootmultiboot)   
        *(.boottext)        
    }

    .bootrodata :
    {
        *(.bootrodata)
    }

    .bootdata :
    {
        *(.bootdata)
    }

    .bootbss ALIGN(0x1000):
    {
        *(.bootbss)
    } 

	. = ALIGN(0x1000);

	_boot_physical_end = .;

	. += 0xFFFFFFFF80000000;

	_kernel_start = .;
	kernel_physical_start = . - 0xFFFFFFFF80000000;

	.text : AT(_boot_physical_end)
	{
		*(.multiboot)
		*(.text)
	}

	. = ALIGN(0x1000);

	.rodata : AT ( (LOADADDR (.text) + SIZEOF (.text) + 0xFFF) & 0xFFFFFFFFFFFFF000 )
	{
		*(.rodata)
	}

	. = ALIGN(0x1000);

	.data : AT ( (LOADADDR (.rodata) + SIZEOF (.rodata) + 0xFFF) & 0xFFFFFFFFFFFFF000 )
	{
		*(.data)
	}

	    /* C++ constructor / destructor bölümleri */
    .init_array : AT ( (LOADADDR (.data) + SIZEOF (.data) + 0xFFF) & 0xFFFFFFFFFFFFF000 )
    {
        PROVIDE(__init_array_start = .);
        KEEP (*(SORT_BY_INIT_PRIORITY(.init_array.*)))
        KEEP (*(.init_array))
        PROVIDE(__init_array_end = .);
    }

    .fini_array : AT ( (LOADADDR (.init_array) + SIZEOF (.init_array) + 0xFFF) & 0xFFFFFFFFFFFFF000 )
    {
        PROVIDE(__fini_array_start = .);
        KEEP (*(SORT_BY_INIT_PRIORITY(.fini_array.*)))
        KEEP (*(.fini_array))
        PROVIDE(__fini_array_end = .);
    }

    .ctors : AT ( (LOADADDR (.fini_array) + SIZEOF (.fini_array) + 0xFFF) & 0xFFFFFFFFFFFFF000 )
    {
        PROVIDE(__ctors_start = .);
        KEEP (*(.ctors))
        KEEP (*(SORT(.ctors.*)))
        PROVIDE(__ctors_end = .);
    }

    .dtors : AT ( (LOADADDR (.ctors) + SIZEOF (.ctors) + 0xFFF) & 0xFFFFFFFFFFFFF000 )
    {
        PROVIDE(__dtors_start = .);
        KEEP (*(.dtors))
        KEEP (*(SORT(.dtors.*)))
        PROVIDE(__dtors_end = .);
    }

	. = ALIGN(0x1000);

	.bss : AT ( (LOADADDR (.dtors) + SIZEOF (.dtors) + 0xFFF) & 0xFFFFFFFFFFFFF000 )
	{
		*(.bss)
		*(.bss.*)
	}

	_kernel_end = .;

	kernel_physical_end = (LOADADDR (.bss) + SIZEOF (.bss) + 0xFFF) & 0xFFFFFFFFFFFFF000;

}