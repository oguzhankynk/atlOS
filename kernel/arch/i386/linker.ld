/* The bootloader will look at this image and start execution at the symbol
   designated at the entry point. */
ENTRY(_start)

/* Tell where the various sections of the object files will be put in the final
   kernel image. */
SECTIONS
{
	/* Begin putting sections at 1 MiB, a conventional place for kernels to be
	   loaded at by the bootloader. */
	. = 0x100000;

	/* First put the multiboot header, as it is required to be put very early
	   early in the image or the bootloader won't recognize the file format.
	 */


	 .multiboot ALIGN(4K): {
         KEEP(*(.multiboot))   /* first the multiboot header */
    }

	.note.gnu.build-id ALIGN(4K): {
        *(.note.gnu.build-id)
    }

    .boot ALIGN(4K) : {
        *(.boot)
    }

	. += 0xC0000000;

	/* .text section. */
	.text ALIGN(4K): AT(ADDR(.text) - 0xC0000000){
        *(.text)
    }

	/* Read-only data. */
    .rodata ALIGN(4K): AT(ADDR(.rodata) - 0xC0000000){
        *(.rodata)
    }


	/* Read-write data (initialized) */
    .data ALIGN(4K): AT(ADDR(.data) - 0xC0000000){
        *(.data)
    }


	/* Read-write data (uninitialized) and stack */
    .bss ALIGN(4K): AT(ADDR(.bss) - 0xC0000000){
        *(COMMON)
        *(.bss)
    }

	_kernel_end = .;

}
