#include <Memory/VirtualMemory.hpp>

#include <string.h>

#include <kernel/panic.h>
#include <kernel/Logger.hpp>
#include <Memory/Memory.hpp>
#include <Memory/MemoryConstants.hpp>
#include <Memory/PhysicalMemory.hpp>
#include <arch/x86_64/MMU/mmu.inc>

namespace Memory
{
    namespace VirtualMemory
    {
        /// Enable / Disable logs
        static constexpr LogFlag DBG_LOG = LogFlag::NoLog;
        static constexpr LogFlag INF_LOG = LogFlag::NoLog;
        static constexpr LogFlag ERR_LOG = LogFlag::NoLog;

        /**
         * @brief Page directories defined at boot.S
         *
         */
        extern "C" PageDirectoryEntry64 pml4[512];
        extern "C" PageDirectoryEntry64 low_pdpt[512];
        extern "C" PageDirectoryEntry64 high_pdpt[512];
        extern "C" PageDirectoryEntry64 low_page_directory_table[512];
        extern "C" PageDirectoryEntry64 high_page_directory_table[512];

        /**
         * @brief Get page index from virtual address
         *
         * @param virt_addr
         * @return uint64_t
         */
        static inline uint64_t PAGE_INDEX(const void *virt_addr)
        {
            uintptr_t addr = (uintptr_t)virt_addr;
            if (addr < KERNEL_VIRTUAL_LOAD_OFFSET)
            {
                return 0; // Invalid address for kernel space
            }
            return ((addr - KERNEL_VIRTUAL_LOAD_OFFSET) >> 21); // 2MB pages
        }

        /**
         * @brief Get virtual address from page index
         *
         * @param index
         * @return void*
         */
        static inline void *PAGE_INDEX_TO_VIRTADDR(uint64_t index)
        {
            return (void *)(KERNEL_VIRTUAL_LOAD_OFFSET + (index << 21));
        }

        /**
         * @brief PD (Page Directory) address calculation
         *        1 PD = 512 × 2MB = 1GB
         */
        static inline PageDirectoryEntry64 *getPageDirectory(uint64_t pdptIndex)
        {
            // each PDPT entry has 512 PDEs (1GB)
            return (PageDirectoryEntry64 *)((uintptr_t)high_page_directory_table + (pdptIndex * PAGE_DIRECTORY_SIZE));
        }

        void init()
        {
            LOG_MSG(INF_LOG, "VMM: Init done. PML4 and initial 2MB mappings are ready.\n");
        }

        /**
         * @brief Map pages
         *
         * @param page_index
         * @param num_pages
         * @return uint64_t
         */
        uint64_t mapPages(uint64_t page_index, uint64_t num_pages)
        {
            uint64_t pdpt_index = page_index / 512;
            uint64_t pd_index = page_index % 512;
            PageDirectoryEntry64 *pd = getPageDirectory(pdpt_index);

            LOG_MSG(DBG_LOG, "VMM: mapPages start=%llu pages=%llu\n", page_index, num_pages);

            for (uint64_t i = 0; i < num_pages; ++i)
            {
                uint64_t phys = (uintptr_t)PhysicalMemory::AllocateBlock(1);
                if (!phys)
                    kernel_panic("VMM: mapPages: Out of physical memory!");

                uint64_t entry_val = (phys & 0xFFFF'FFFF'FFE0'0000ULL) // 2MB align
                                     | MMU_PRESENT | MMU_WRITABLE | MMU_PDE_TWO_MB;

                ((uint64_t *)pd)[pd_index] = entry_val;

                // invalidate TLB for this virtual address
                void *virt = PAGE_INDEX_TO_VIRTADDR(page_index + i);
                asm volatile("invlpg (%0)" : : "r"(virt) : "memory");

                pd_index++;
                if (pd_index == 512)
                {
                    pdpt_index++;
                    pd_index = 0;
                    pd = getPageDirectory(pdpt_index);
                }
            }

            return num_pages;
        }

        uint64_t unmapPages(uint64_t page_index, uint64_t num_pages)
        {
            uint64_t pdpt_index = page_index / 512;
            uint64_t pd_index = page_index % 512;
            PageDirectoryEntry64 *pd = getPageDirectory(pdpt_index);

            for (uint64_t i = 0; i < num_pages; ++i)
            {
                uint64_t entry = ((uint64_t *)pd)[pd_index];
                if (!(entry & MMU_PRESENT))
                    kernel_panic("VMM: unmapPages: page not present!");

                void *phys_addr = (void *)(entry & 0xFFFF'FFFF'FFE0'0000ULL);
                PhysicalMemory::FreeBlocks(phys_addr, 1);

                ((uint64_t *)pd)[pd_index] = 0;

                void *virt = PAGE_INDEX_TO_VIRTADDR(page_index + i);
                asm volatile("invlpg (%0)" : : "r"(virt) : "memory");

                pd_index++;
                if (pd_index == 512)
                {
                    pdpt_index++;
                    pd_index = 0;
                    pd = getPageDirectory(pdpt_index);
                }
            }

            return num_pages;
        }

        /// Simple contiguous page allocator
        static uint64_t findContiguousFreePages(uint64_t num_pages)
        {

            uint64_t start_index = PAGE_INDEX(&kernel_virtual_end);
            uint64_t max_pages = 512ULL * 512ULL * 256ULL; // 512 PML4 * 512 PDPT * 512 PDE teorik sınır

            for (uint64_t i = start_index; i < max_pages; ++i)
            {
                bool free = true;
                for (uint64_t j = 0; j < num_pages; ++j)
                {
                    uint64_t pdpt_index = (i + j) / 512;
                    uint64_t pd_index = (i + j) % 512;
                    PageDirectoryEntry64 *pd = getPageDirectory(pdpt_index);
                    if (((uint64_t *)pd)[pd_index] & MMU_PRESENT)
                    {
                        free = false;
                        i += j;
                        break;
                    }
                }
                if (free)
                    return i;
            }

            kernel_panic("VMM: findContiguousFreePages: no free area!");
            return 0;
        }

        void *allocatePage(uint64_t num_pages)
        {
            uint64_t start_index = findContiguousFreePages(num_pages);
            mapPages(start_index, num_pages);
            return PAGE_INDEX_TO_VIRTADDR(start_index);
        }

        int freePage(const void *addr, uint64_t num_pages)
        {
            unmapPages(PAGE_INDEX(addr), num_pages);
            return 0;
        }
    }; // namespace VirtualMemory
}; // namespace Memory