#include <Memory/Memory.hpp>

#include <stdio.h>
#include <string.h>

#include <Memory/MemoryConstants.hpp>
#include <Memory/PhysicalMemory.hpp>
#include <Memory/VirtualMemory.hpp>

#include <kernel/sys/multiboot.h>

namespace Memory
{

    /**
     * @brief Page directory defined at boot.S
     *
     */
    extern "C" VirtualMemory::PageDirectory page_directory[PAGE_DIR_SIZE];

    /**
     * @brief Bitmap for manege physical memory
     *
     */
    static uint8_t bitmap[4 * 128 * 1024 * 1024UL / PAGE_SIZE];

    /**
     * @brief Start address of the kernel.
     *
     * This variable is defined at linker.ld and represents the start address of
     * the kernel in virtual memory.
     */
    extern "C" uint32_t _kernel_start;

    /**
     * @brief End address of the kernel.
     *
     * This variable is defined at linker.ld and represents the end address of
     * the kernel in virtual memory.
     */
    extern "C" uint32_t _kernel_end;

    /**
     * @brief Physical start address of the kernel.
     *
     * This variable is defined at linker.ld and represents the start address of
     * the kernel in physical memory.
     */
    extern "C" uint32_t kernel_physical_start;

    /**
     * @brief Physical end address of the kernel.
     *
     * This variable is defined at linker.ld and represents the end address of
     * the kernel in physical memory.
     */
    extern "C" uint32_t kernel_physical_end;

    /**
     * @brief
     *
     * @param memHigh
     * @param physicalAllocStart
     */
    void Init(const multiboot_info_t *bootInfo)
    {
        printf("Kernel virt: \t start: 0x%x \tend: 0x%x \n", &_kernel_start, &_kernel_end);
        printf("Kernel  phy: \t start: 0x%x \tend: 0x%x \n", &kernel_physical_start, &kernel_physical_end);

        VirtualMemory::Init();

        auto totalMemorySizeKB = bootInfo->mem_lower + bootInfo->mem_upper;
        printf("Total memory size: 0x%x KB\n", totalMemorySizeKB);

        PhysicalMemory::Init(bitmap, totalMemorySizeKB * 1024, PAGE_SIZE);

        for (int i = 0; i < bootInfo->mmap_length; i += sizeof(multiboot_mmap_entry))
        {
            auto *mmmt = (multiboot_mmap_entry *)(bootInfo->mmap_addr + i);

            uint32_t base_addr = ((uint64_t)mmmt->addr_high << 32) | mmmt->addr_low;
            uint32_t length = ((uint64_t)mmmt->len_high << 32) | mmmt->len_low;

            printf("Addr: 0x%x | Length: 0x%x | size: 0x%x | Type: 0x%x \n",
                   base_addr,
                   length,
                   mmmt->size, mmmt->type);

            if (mmmt->type == MULTIBOOT_MEMORY_AVAILABLE)
            {
                PhysicalMemory::AddMemoryRegion((void *)base_addr, length);
            }
        }

        PhysicalMemory::RemoveMemoryRegion(0, 1024 * 1024);
        PhysicalMemory::RemoveMemoryRegion(&kernel_physical_start, (uint32_t)&kernel_physical_end - (uint32_t)&kernel_physical_start);
    }

    /**
     * @brief Converts a virtual address to a physical address.
     *
     * @param virtualaddr The virtual address to be converted.
     * @return void* The corresponding physical address, or nullptr if the address is not present.
     */
    void *VirtToPhysAddr(void *virtualaddr)
    {
        using namespace VirtualMemory;

        uint32_t pdindex = (unsigned long)virtualaddr >> 22 & 0x3FF;
        uint32_t ptindex = (unsigned long)virtualaddr >> 12 & 0x3FF;
        uint32_t offset = (uint32_t)virtualaddr & 0xFFF;

        if (!page_directory[pdindex].present)
        {
            return 0;
        }

        uint32_t page_table_phy = (uint32_t)(page_directory[pdindex].table_addr << 12);

        PageTableEntry *page_table = (PageTableEntry *)(page_table_phy + KERNEL_LOAD_ADDR);

        if (!page_table[ptindex].present)
        {
            return 0;
        }

        uint32_t page_addr = page_table[ptindex].frame << 12;

        return (void *)(page_addr + offset);
    }

} // namespace Memory
