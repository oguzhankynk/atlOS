#include <Memory/Memory.hpp>

#include <stdio.h>
#include <string.h>

#include <Memory/MemoryConstants.hpp>
#include <Memory/PhysicalMemory.hpp>
#include <Memory/VirtualMemory.hpp>

#include <kernel/sys/multiboot.h>
#include <kernel/sys/handlers.hpp>

namespace Memory
{

    /**
     * @brief Page directory defined at boot.S
     *
     */
    extern "C" VirtualMemory::PageDirectoryEntry page_directory[PAGE_DIR_SIZE];

    /**
     * @brief
     *
     * @param memHigh
     * @param physicalAllocStart
     */
    void Init(const multiboot_info_t *bootInfo)
    {
        printf("Kernel virt: \t start: 0x%x \tend: 0x%x \n", (uint32_t)&_kernel_start, (uint32_t)&_kernel_end);
        printf("Kernel  phy: \t start: 0x%x \tend: 0x%x \n", (uint32_t)&kernel_physical_start, (uint32_t)&kernel_physical_end);

        PhysicalMemory::Init();
        VirtualMemory::Init();

        auto totalMemorySizeKB = bootInfo->mem_lower + bootInfo->mem_upper;
        printf("Total memory size: 0x%x KB\n", totalMemorySizeKB);


        for (int i = 0; i < bootInfo->mmap_length; i += sizeof(multiboot_mmap_entry))
        {
            auto *mmmt = (multiboot_mmap_entry *)(bootInfo->mmap_addr + i);

            uint32_t base_addr = ((uint64_t)mmmt->addr_high << 32) | mmmt->addr_low;
            uint32_t length = ((uint64_t)mmmt->len_high << 32) | mmmt->len_low;

            auto getType = [](multiboot_uint32_t type) -> const char *
            {
                switch (type)
                {
                case MULTIBOOT_MEMORY_AVAILABLE:
                    return "AVAILABLE";
                case MULTIBOOT_MEMORY_RESERVED:
                    return "RESERVED";
                case MULTIBOOT_MEMORY_ACPI_RECLAIMABLE:
                    return "ACPI_RECLAIMABLE";
                case MULTIBOOT_MEMORY_NVS:
                    return "NVS";
                case MULTIBOOT_MEMORY_BADRAM:
                    return "BADRAM";
                default:
                    return "UNKNOWN"; // Bilinmeyen değerler için
                }
            };


            if (mmmt->type == MULTIBOOT_MEMORY_AVAILABLE)
            {
                printf("Addr: 0x%x | Length: 0x%x | size: 0x%x | Type: %s \n",
                    base_addr,
                    length,
                    mmmt->size,
                    getType(mmmt->type));
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
        uint32_t pdindex = (unsigned long)virtualaddr >> 22;
        uint32_t ptindex = (unsigned long)virtualaddr >> 12 & 0x3FF;
        uint32_t offset = (uint32_t)virtualaddr & 0xFFF;

        if (!page_directory[pdindex].present)
        {
            return 0;
        }

        VirtualMemory::PageTableEntry *page_table = (VirtualMemory::PageTableEntry *)VirtualMemory::getPageTableVirtAddr(pdindex);

        if (!page_table[ptindex].present)
        {
            return 0;
        }

        uint32_t page_addr = page_table[ptindex].frame << 12;

        return (void *)(page_addr + offset);
    }

    void map(void *physicaladdr, void *virtualaddr)
    {
        if ((uint32_t)virtualaddr % PAGE_SIZE != 0)
            return;

        uint32_t pdindex = (unsigned long)virtualaddr >> 22 ;
        uint32_t ptindex = (unsigned long)virtualaddr >> 12 & 0x3FF;

        VirtualMemory::PageDirectoryEntry &pd_entry = page_directory[pdindex];

        if (!pd_entry.present)
        {
            VirtualMemory::PageTableEntry *new_pt = (VirtualMemory::PageTableEntry *)PhysicalMemory::AllocateBlock(1);

            if (new_pt == nullptr)
                return;

            memset(new_pt, 0, PAGE_SIZE);

            pd_entry = (uint32_t)new_pt | 3; // present , rw
        }

        
        VirtualMemory::PageTableEntry *page_table = (VirtualMemory::PageTableEntry *)VirtualMemory::getPageTableVirtAddr(pdindex);

        if (!page_table[ptindex].present)
        {
            page_table[ptindex] = (uint32_t)physicaladdr | 3;
        }
        else
        {
            printf("page is already mapped \n");
        }
    }

    void unmap(void *virtualaddr)
    {
        if ((uint32_t)virtualaddr % PAGE_SIZE != 0)
            return;

        uint32_t pdindex = (unsigned long)virtualaddr >> 22 & 0x3FF;
        uint32_t ptindex = (unsigned long)virtualaddr >> 12 & 0x3FF;

        VirtualMemory::PageDirectoryEntry &pd_entry = page_directory[pdindex];

        if (pd_entry.present)
        {
            VirtualMemory::PageTableEntry *page_table = (VirtualMemory::PageTableEntry *)VirtualMemory::getPageTableVirtAddr(pdindex);
            if (page_table[ptindex].present)
            {
                page_table[ptindex] = 0;
                asm volatile("invlpg (%0)" : : "a"(virtualaddr));
            }

            int unpresent_pte_counter{};
            for (; unpresent_pte_counter < PAGE_TABLE_SIZE; unpresent_pte_counter++)
            {
                if (page_table[unpresent_pte_counter].present)
                    break;
            }

            if (unpresent_pte_counter == PAGE_TABLE_SIZE)
            {
                PhysicalMemory::FreeBlocks((void *)(page_directory[pdindex].table_addr << 12), 1);
                page_directory[pdindex].rw = 1;
                page_directory[pdindex].present = 0;
            }
        }
    }

} // namespace Memory
