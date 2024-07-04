#include <Memory/Memory.hpp>

#include <stdio.h>
#include <string.h>

#include <Memory/PhysicalMemory.hpp>
#include <kernel/sys/multiboot.h>

namespace Memory
{

    /**
     * @brief Size of a single memory page.
     *
     */
    static constexpr uint32_t PAGE_SIZE = 4096;

    /**
     * @brief Number of entries per page directory.
     */
    static constexpr uint32_t PAGE_DIR_SIZE = 1024;

    /**
     * @brief Number of entries per page table.
     */
    static constexpr uint32_t PAGE_TABLE_SIZE = 1024;

    /**
     * @brief Bitmap for manege physical memory
     *
     */
    static uint8_t bitmap[4 * 128 * 1024 * 1024UL / PAGE_SIZE];

    static constexpr uint32_t KERNEL_LOAD_ADDR = (uint32_t)0xC0000000;
    /**
     * @brief Page directory defined at boot.S
     *
     */
    extern "C" PageDirectory page_directory[PAGE_DIR_SIZE];

    /**
     * @brief First page table defined at boot.S
     *
     */
    extern "C" PageTableEntry first_page_table[PAGE_TABLE_SIZE];

    PageTableEntry temp_page_table[PAGE_TABLE_SIZE] __attribute__((aligned(4096)));

    /**
     * @brief Start address of the kernel.
     *
     * This variable is defined at linker.ld and represents the start address of
     * the kernel in memory.
     */
    extern "C" uint32_t _kernel_start;

    /**
     * @brief End address of the kernel.
     *
     * This variable is defined at linker.ld and represents the end address of
     * the kernel in memory.
     */
    extern "C" uint32_t _kernel_end;

    extern "C" uint32_t kernel_physical_start;
    extern "C" uint32_t kernel_physical_end;

    void loadPageDirectory(uint32_t *page_directory);
    void enablePaging();

    /**
     * @brief Construct a new Page PageDirectory object
     *
     * @param value
     */
    PageDirectory::PageDirectory(uint32_t value)
    {
        present = (value >> 0) & 0x1;
        rw = (value >> 1) & 0x1;
        user = (value >> 2) & 0x1;
        pwt = (value >> 3) & 0x1;
        pcd = (value >> 4) & 0x1;
        accessed = (value >> 5) & 0x1;
        reserved = (value >> 6) & 0x1;
        page_size = (value >> 7) & 0x1;
        ignored = (value >> 8) & 0xF;
        table_addr = (value >> 12) & 0xFFFFF;
    }

    /**
     * @brief Construct a new PageTableEntry object
     *
     * @param value
     */
    PageTableEntry::PageTableEntry(uint32_t value)
    {
        present = (value >> 0) & 0x1;
        rw = (value >> 1) & 0x1;
        user = (value >> 2) & 0x1;
        pwt = (value >> 3) & 0x1;
        pcd = (value >> 4) & 0x1;
        accessed = (value >> 5) & 0x1;
        dirty = (value >> 6) & 0x1;
        pat = (value >> 7) & 0x1;
        global = (value >> 8) & 0x1;
        ignored = (value >> 9) & 0x7;
        frame = (value >> 12) & 0xFFFFF;
    }

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

        memset(temp_page_table, 0, sizeof(temp_page_table));

        for (uint32_t addr = 0, i = 0; addr < 1024 * 1024; addr += PAGE_SIZE, i++)
        {
            temp_page_table[i] = addr | 3;
        }

        page_directory[0] = (uint32_t)get_physaddr((void *)temp_page_table) | 3;

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
     * @brief Get the Page Size
     *
     */
    uint32_t getPageSize()
    {
        return PAGE_SIZE;
    }

    /**
     * @brief Load the page directory base register (CR3) with the given page directory.
     *
     * This function sets the CR3 register with the address of the provided page directory.
     * Setting the CR3 register updates the page directory base, which is used by the CPU
     * for translating virtual addresses to physical addresses.
     *
     * @param page_directory A pointer to the page directory to load into CR3.
     */
    void loadPageDirectory(uint32_t *page_directory)
    {
        __asm__ __volatile__(
            "movl %0, %%cr3" // %0 placeholder, replaced by page_directory
            :
            : "r"(page_directory) // Input operand as page_directory
            : "memory"            // Indicates that the inline assembly has memory side effects
        );
    }

    /**
     * @brief Enable paging by setting the PG bit (bit 31) in the CR0 register.
     *
     * This function modifies the CR0 register to enable paging. It sets the most
     * significant bit (PG bit) in the CR0 register, which tells the CPU to start using
     * the page tables for address translation.
     */
    void enablePaging()
    {
        __asm__ __volatile__(
            "mov %%cr0, %%eax;"      // Load the value of CR0 into EAX register
            "or $0x80000001, %%eax;" // Set the most significant bit in EAX register (paging bit)
            "mov %%eax, %%cr0;"      // Move the modified value back into CR0 register
            :
            :
            : "eax", "memory" // Indicates that EAX register is modified and memory has side effects
        );
    }

    /**
     * @brief Converts a virtual address to a physical address.
     *
     * @param virtualaddr The virtual address to be converted.
     * @return void* The corresponding physical address, or nullptr if the address is not present.
     */
    void *get_physaddr(void *virtualaddr)
    {
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
