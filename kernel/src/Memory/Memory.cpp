#include <Memory/Memory.hpp>
#include <Memory/PhysicalMemory.hpp>
#include <kernel/sys/multiboot.h>
#include <stdio.h>

#define CHECK_FLAG(flags, bit) ((flags) & (1 << (bit)))

namespace Memory
{
    void InitMemory(void *boot_info_addr)
    {
        multiboot_info_t* boot_info = (multiboot_info_t*)boot_info_addr;

        /* Print out the flags. */
        printf("flags = 0x%x\n", (unsigned)boot_info->flags);

        /* Are mem_* valid? */
        if (CHECK_FLAG(boot_info->flags, 0))
        {
            uint32_t total_memory_kb = (boot_info->mem_lower + boot_info->mem_upper);

            printf("mem_lower = %uKB, mem_upper = %uKB total_memory = %uKB\n",
                   (unsigned)boot_info->mem_lower, (unsigned)boot_info->mem_upper, total_memory_kb);
        }

        
    constexpr uint32_t MEMMAP_AREA = 0x30000;

    PhysicalMemory::getInstance().Init((void*)MEMMAP_AREA, (uint32_t)(boot_info->mem_lower + boot_info->mem_upper));

    /* Is boot_device valid? */
    if (CHECK_FLAG(boot_info->flags, 1))
        printf("boot_device = 0x%x\n", (unsigned)boot_info->boot_device);

    /* Is the command line passed? */
    if (CHECK_FLAG(boot_info->flags, 2))
        printf("cmdline = %s\n", (char *)boot_info->cmdline);

    /* Are mods_* valid? */
    if (CHECK_FLAG(boot_info->flags, 3))
    {   
        int i;
        multiboot_module_t *mod;

        printf("mods_count = %d, mods_addr = 0x%x\n",
               (int)boot_info->mods_count, (int)boot_info->mods_addr);

        for (i = 0, mod = (multiboot_module_t *)boot_info->mods_addr;
             i < boot_info->mods_count;
             i++, mod++)
            printf(" mod_start = 0x%x, mod_end = 0x%x, cmdline = %s\n",
                   (unsigned)mod->mod_start,
                   (unsigned)mod->mod_end,
                   (char *)mod->cmdline);
    }

    /* Bits 4 and 5 are mutually exclusive! */
    if (CHECK_FLAG(boot_info->flags, 4) && CHECK_FLAG(boot_info->flags, 5))
    {
        printf("Both bits 4 and 5 are set.\n");
        return;
    }

    /* Is the symbol table of a.out valid? */
    if (CHECK_FLAG(boot_info->flags, 4))
    {
        multiboot_aout_symbol_table_t *multiboot_aout_sym = &(boot_info->u.aout_sym);

        printf("multiboot_aout_symbol_table: tabsize = 0x%0x, "
               "strsize = 0x%x, addr = 0x%x\n",
               (unsigned)multiboot_aout_sym->tabsize,
               (unsigned)multiboot_aout_sym->strsize,
               (unsigned)multiboot_aout_sym->addr);
    }

    /* Is the section header table of ELF valid? */
    if (CHECK_FLAG(boot_info->flags, 5))
    {
        multiboot_elf_section_header_table_t *multiboot_elf_sec = &(boot_info->u.elf_sec);

        printf("multiboot_elf_sec: num = %u, size = 0x%x,"
               " addr = 0x%x, shndx = 0x%x\n",
               (unsigned)multiboot_elf_sec->num, (unsigned)multiboot_elf_sec->size,
               (unsigned)multiboot_elf_sec->addr, (unsigned)multiboot_elf_sec->shndx);
    }

    /* Are mmap_* valid? */
    if (CHECK_FLAG(boot_info->flags, 6))
    {
        for(int i=0; i<boot_info->mmap_length; i+= sizeof(multiboot_mmap_entry))
        {
            auto *mmmt = (multiboot_mmap_entry*)(boot_info->mmap_addr + i);

            printf("Addr: %x %x | Length: %x %x | size: %x | Type: %x \n",
            mmmt->addr_low, mmmt->addr_high,
            mmmt->len_low, mmmt->len_high,
            mmmt->size, mmmt->type);
        }
    }
    
    PhysicalMemory::getInstance().RemoveMemoryRegion((void*)0x1000, 0xB000);                            // Reserve all memory below C000h for the kernel/OS
    PhysicalMemory::getInstance().RemoveMemoryRegion((void*)MEMMAP_AREA, PhysicalMemory::getInstance().GetMaxBlockSize());

    }

} // namespace Memory
