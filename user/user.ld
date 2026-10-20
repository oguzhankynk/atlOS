/*
 * Linker script for atlOS user programs: a static ELF64 executable at
 * userBase (0x0000008000000000, PML4[1]), code and data in separate 2 MB pages
 * (protection is per page). Link with -z max-page-size=0x200000.
 */
OUTPUT_FORMAT(elf64-x86-64)
ENTRY(_start)

SECTIONS
{
    /* The first segment starts at userBase and holds the ELF headers too,
       so its file offset is 0 and the file needs no padding */
    . = 0x0000008000000000 + SIZEOF_HEADERS;

    .text : { *(.text.start) *(.text .text.*) }
    .rodata : { *(.rodata .rodata.*) }

    /* Data: the next 2 MB page, at the same offset inside the page as in the file */
    . = ALIGN(0x200000) + (. & (0x200000 - 1));

    .data : { *(.data .data.*) }
    .bss : { *(.bss .bss.*) *(COMMON) }

    /DISCARD/ : { *(.comment) *(.eh_frame) *(.note.*) }
}
