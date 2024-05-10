/**
 * @file kernel.cpp
 * @author Oguzhan KAYNAK
 * @brief Kernel implementation file containing kernel initialization and utility functions.
 *
 * This file contains the implementation of kernel initialization and various utility functions
 * used by the kernel during runtime. It includes functions for hardware initialization,
 * interrupt handling, memory management, and other essential operations.
 * Additionally, it may contain device drivers, system calls, and other kernel-specific functionalities.
 *
 * Copyright © 2024 Oguzhan KAYNAK
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the “Software”), to deal in the Software
 * without  restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit
 * persons to whom the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
 * PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */
#include <stdio.h>
#include <string.h>

#include <kernel/tty.h>
#include <kernel/BIOS/BIOS.h>
#include <kernel/sys/multiboot.h>
#include <kernel/panic.h>

#include <arch/i386/GDT/gdt.hpp>
#include <arch/i386/IDT/idt.hpp>

#include <kernel/banner.h>

#define CHECK_FLAG(flags, bit) ((flags) & (1 << (bit)))

/**
 * @brief firs point of ParsOS
 *
 */
extern "C" void kernel_main(uint32_t magic, uint32_t addr)
{
    GDT::getInstance().Init();
    IDT::getInstance().Init();
    terminal_initialize();

    ParsOSBanner();

    asm volatile("int $0x00");
    if (magic != MULTIBOOT_BOOTLOADER_MAGIC)
    {
        kernel_panic("magic number is invalid.");
    }

    multiboot_info_t* mbi = (multiboot_info_t *)addr;

    /* Print out the flags. */
    printf("flags = 0x%x\n", (unsigned)mbi->flags);

    /* Are mem_* valid? */
    if (CHECK_FLAG(mbi->flags, 0))
        printf("mem_lower = %uKB, mem_upper = %uKB\n",
               (unsigned)mbi->mem_lower, (unsigned)mbi->mem_upper);

    /* Is boot_device valid? */
    if (CHECK_FLAG(mbi->flags, 1))
        printf("boot_device = 0x%x\n", (unsigned)mbi->boot_device);

    /* Is the command line passed? */
    if (CHECK_FLAG(mbi->flags, 2))
        printf("cmdline = %s\n", (char *)mbi->cmdline);

    /* Are mods_* valid? */
    if (CHECK_FLAG(mbi->flags, 3))
    {
        multiboot_module_t *mod;
        int i;

        printf("mods_count = %d, mods_addr = 0x%x\n",
               (int)mbi->mods_count, (int)mbi->mods_addr);
        for (i = 0, mod = (multiboot_module_t *)mbi->mods_addr;
             i < mbi->mods_count;
             i++, mod++)
            printf(" mod_start = 0x%x, mod_end = 0x%x, cmdline = %s\n",
                   (unsigned)mod->mod_start,
                   (unsigned)mod->mod_end,
                   (char *)mod->cmdline);
    }

    /* Bits 4 and 5 are mutually exclusive! */
    if (CHECK_FLAG(mbi->flags, 4) && CHECK_FLAG(mbi->flags, 5))
    {
        printf("Both bits 4 and 5 are set.\n");
        return;
    }

    /* Is the symbol table of a.out valid? */
    if (CHECK_FLAG(mbi->flags, 4))
    {
        multiboot_aout_symbol_table_t *multiboot_aout_sym = &(mbi->u.aout_sym);

        printf("multiboot_aout_symbol_table: tabsize = 0x%0x, "
               "strsize = 0x%x, addr = 0x%x\n",
               (unsigned)multiboot_aout_sym->tabsize,
               (unsigned)multiboot_aout_sym->strsize,
               (unsigned)multiboot_aout_sym->addr);
    }

    /* Is the section header table of ELF valid? */
    if (CHECK_FLAG(mbi->flags, 5))
    {
        multiboot_elf_section_header_table_t *multiboot_elf_sec = &(mbi->u.elf_sec);

        printf("multiboot_elf_sec: num = %u, size = 0x%x,"
               " addr = 0x%x, shndx = 0x%x\n",
               (unsigned)multiboot_elf_sec->num, (unsigned)multiboot_elf_sec->size,
               (unsigned)multiboot_elf_sec->addr, (unsigned)multiboot_elf_sec->shndx);
    }

    /* Are mmap_* valid? */
    if (CHECK_FLAG(mbi->flags, 6))
    {
        multiboot_memory_map_t *mmap;

        printf("mmap_addr = 0x%x, mmap_length = 0x%x\n",
               (unsigned)mbi->mmap_addr, (unsigned)mbi->mmap_length);
        for (mmap = (multiboot_memory_map_t *)mbi->mmap_addr;
             (unsigned long)mmap < mbi->mmap_addr + mbi->mmap_length;
             mmap = (multiboot_memory_map_t *)((unsigned long)mmap + mmap->size + sizeof(mmap->size)))
            printf(" size = 0x%x, base_addr = 0x%x%08x,"
                   " length = 0x%x%08x, type = 0x%x\n",
                   (unsigned)mmap->size,
                   (unsigned)(mmap->addr >> 32),
                   (unsigned)(mmap->addr & 0xffffffff),
                   (unsigned)(mmap->len >> 32),
                   (unsigned)(mmap->len & 0xffffffff),
                   (unsigned)mmap->type);
    }

    /* Loop through the memory map and display the values */
    // for (int i = 0; i < mbd->mmap_length; i += sizeof(multiboot_memory_map_t))
    // {
    //     multiboot_memory_map_t *mmmt =
    //         (multiboot_memory_map_t *)(mbd.mmap_addr + i);

    //     printf("Start Addr: 0x%llx | Length: 0x%llx | Size: 0x%x | Type: %u\n",
    //            mmmt->addr, mmmt->len, mmmt->size, mmmt->type);

    //     if (mmmt->type == MULTIBOOT_MEMORY_AVAILABLE)
    //     {
    //         /*
    //          * Do something with this memory block!
    //          * BE WARNED that some of memory shown as availiable is actually
    //          * actively being used by the kernel! You'll need to take that
    //          * into account before writing to memory!
    //          */
    //     }
    // }

    BIOSInit();

    while (1)
        ;
}
