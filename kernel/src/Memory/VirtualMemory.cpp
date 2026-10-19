#include <Memory/VirtualMemory.hpp>

#include <string.h>

#include <kernel/panic.h>
#include <kernel/Logger.hpp>
#include <Memory/Memory.hpp>
#include <Memory/MemoryConstants.hpp>
#include <Memory/PhysicalMemory.hpp>
#include <arch/x86_64/MMU/mmu.inc>

/*
    x86_64 48-bit canonical virtual address layout for 2 MB huge pages
    with recursive PML4 mapping (PML4 → PDPT → PD → [2MB PAGE]):

    63  . . . 48 | 47     39 | 38     30 | 29     21 | 20     0
    -----------------------------------------------------------
    Sign extend  |   PML4    |   PDPT    |    PD     | Page offset

    Breakdown:
    - Bits 47..39 : PML4 index (9 bits, 512 entries)
    - Bits 38..30 : PDPT index (9 bits, 512 entries)
    - Bits 29..21 : PD index   (9 bits, 512 entries)
    - Bits 20..0  : Offset within 2 MB huge page (21 bits)

    Recursive mapping design (THIS KERNEL):
    - Kernel virtual space lives entirely in PML4[511]
    - Recursive page-table mapping uses PML4[510]
    - This avoids overwriting kernel mappings

    Recursive mapping effect:
    - PML4[510] → points to the PML4 itself
    - Enables virtual access to:
        * PML4
        * PDPTs
        * PDs
      without using physical addresses

    Notes:
    - PT (page table) level does NOT exist (2 MB pages)
    - PD entry must have page_size = 1
    - Only lower 48 bits are valid (canonical addresses)
    - Bits 63..48 must be sign-extension of bit 47
*/

namespace Memory
{
    namespace VirtualMemory
    {
        /// Enable / Disable logs
        static constexpr LogFlag DBG_LOG = LogFlag::Log;
        static constexpr LogFlag INF_LOG = LogFlag::Log;
        static constexpr LogFlag ERR_LOG = LogFlag::Log;

        /**
         * @brief Page tables defined and identity-mapped in boot.S
         */
        extern "C" PageDirectoryEntry64 pml4[512];
        extern "C" PageDirectoryEntry64 high_pdpt[512];
        extern "C" PageDirectoryEntry64 high_page_directory_table[512];
        extern "C" PageDirectoryEntry64 low_page_directory_table[512];
        extern "C" PageDirectoryEntry64 low_pdpt[512];

        /**
         * Recursive mapping slot.
         * Kernel occupies PML4[511], so recursive MUST NOT use 511.
         */
        constexpr uint64_t RECURSIVE_SLOT = 510;

        // ------------------------------------------------------------
        // CR3 helpers
        // ------------------------------------------------------------

        static inline uint64_t readCr3()
        {
            uint64_t v;
            asm volatile("mov %%cr3, %0" : "=r"(v));
            return v;
        }

        static inline void writeCr3(uint64_t v)
        {
            asm volatile("mov %0, %%cr3" ::"r"(v) : "memory");
        }

        constexpr uint64_t RECURSIVE_PREFIX = 0xFFFF000000000000ULL;

        // ------------------------------------------------------------
        // Recursive mapping virtual address helpers
        // ------------------------------------------------------------

        /**
         * @brief Virtual address of the PML4 itself via recursive mapping
         *
         * VA bits:
         *   [510][510][510][0]
         */
        inline PageDirectoryEntry64 *getPML4Virt()
        {
            return (PageDirectoryEntry64 *)(RECURSIVE_PREFIX |
                                            (RECURSIVE_SLOT << 39) |
                                            (RECURSIVE_SLOT << 30) |
                                            (RECURSIVE_SLOT << 21) |
                                            (RECURSIVE_SLOT << 12));
        }

        /**
         * @brief Get PDPT virtual address for a given PML4 index
         *
         * VA bits:
         *   [510][pml4_index][510][0]
         */
        inline PageDirectoryEntry64 *getPDPTVirt(uint64_t pml4_index)
        {
            return (PageDirectoryEntry64 *)(RECURSIVE_PREFIX |
                                            (RECURSIVE_SLOT << 39) |
                                            (RECURSIVE_SLOT << 30) |
                                            (RECURSIVE_SLOT << 21) |
                                            (pml4_index << 12));
        }

        /**
         * @brief Get PD virtual address for a given PML4 + PDPT index
         *
         * VA bits:
         *   [510][pml4_index][pdpt_index][0]
         *
         * This is the FINAL level for 2 MB pages.
         */
        inline PageDirectoryEntry64 *getPDVirt(uint64_t pml4_index, uint64_t pdpt_index)
        {
            return (PageDirectoryEntry64 *)(RECURSIVE_PREFIX |
                                            (RECURSIVE_SLOT << 39) |
                                            (RECURSIVE_SLOT << 30) |
                                            (pml4_index << 21) |
                                            (pdpt_index << 12));
        }

        // ------------------------------------------------------------
        // Page index helpers (2 MB pages)
        // ------------------------------------------------------------

        static inline uint64_t getPageIndexFromVirtAddr(const void *virt_addr)
        {
            uintptr_t addr = (uintptr_t)virt_addr;
            if (addr < KERNEL_VIRTUAL_LOAD_OFFSET)
                kernel_panic("VMM: invalid kernel virtual address");

            return (addr - KERNEL_VIRTUAL_LOAD_OFFSET) >> 21;
        }

        static inline void *getVirtAddrFromPageIndex(uint64_t index)
        {
            return (void *)(KERNEL_VIRTUAL_LOAD_OFFSET + (index << 21));
        }

        /**
         * @brief Boot-time PD access (static tables from boot.S)
         *
         * NOTE:
         * This does NOT use recursive mapping.
         * It is valid because boot.S identity-maps these tables.
         */
        static inline PageDirectoryEntry64 *getPageDirectory(uint64_t pdptIndex)
        {
            return (PageDirectoryEntry64 *)((uintptr_t)high_page_directory_table +
                                            (pdptIndex * PAGE_DIRECTORY_SIZE));
        }

        // ------------------------------------------------------------
        // Initialization
        // ------------------------------------------------------------

        void init()
        {
            LOG_MSG(INF_LOG, "VMM: Init start. Setting up recursive mapping...\n");

            uint64_t current_cr3 = readCr3() & ~0xFFFULL;

            uint64_t *pml4_raw = (uint64_t *)&pml4[0];
            pml4_raw[RECURSIVE_SLOT] = current_cr3 | 0x03;

            // Full TLB flush is REQUIRED here
            writeCr3(readCr3());

            LOG_MSG(INF_LOG, "VMM: Recursive mapping set at PML4[%llu]: 0x%llx\n", RECURSIVE_SLOT, *(uint64_t *)&pml4[RECURSIVE_SLOT]);
            LOG_MSG(INF_LOG, "VMM: Init done.\n");
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
            PageDirectoryEntry64 *pml4_table = getPML4Virt();

            for (uint64_t i = 0; i < num_pages; ++i)
            {
                uintptr_t v_addr = (uintptr_t)getVirtAddrFromPageIndex(page_index + i);
                uint64_t pml4_idx = (v_addr >> 39) & 0x1FF;
                uint64_t pdpt_idx = (v_addr >> 30) & 0x1FF;
                uint64_t pd_idx = (v_addr >> 21) & 0x1FF;

                printf("Mapping page %llu: VA=0x%llx (PML4=%llu, PDPT=%llu, PD=%llu)\n",
                       page_index + i, v_addr, pml4_idx, pdpt_idx, pd_idx);

                // PDPT check (PML4 entry)
                if (!pml4_table[pml4_idx].present)
                {
                    kernel_panic("Dynamic PDPT creation not implemented yet!");
                }

                PageDirectoryEntry64 *pdpt_table = getPDPTVirt(pml4_idx);
                asm volatile("invlpg (%0)" : : "r"(pdpt_table) : "memory");

                // Page Directory Kontrolü (PDPT içindeki girişe bakıyoruz)
                if (!pdpt_table[pdpt_idx].present)
                {
                    uint64_t new_pd_phys = (uintptr_t)PhysicalMemory::AllocateFrame();
                    if (new_pd_phys == 0)
                        kernel_panic("VMM: out of physical memory for page directory");

                    pdpt_table[pdpt_idx].present = 1;
                    pdpt_table[pdpt_idx].rw = 1;
                    pdpt_table[pdpt_idx].frame_addr = new_pd_phys >> 12;

                    PageDirectoryEntry64 *pd_table = getPDVirt(pml4_idx, pdpt_idx);
                    asm volatile("invlpg (%0)" : : "r"(pd_table) : "memory");

                    memset(pd_table, 0, 4096);
                }

                PageDirectoryEntry64 *pd_table = getPDVirt(pml4_idx, pdpt_idx);
                uint64_t phys = (uintptr_t)PhysicalMemory::AllocateBlock(1);
                if (phys == 0)
                    kernel_panic("VMM: out of physical memory");

                uint64_t *raw_pd = (uint64_t *)pd_table;
                raw_pd[pd_idx] = (1ULL << 0) | (1ULL << 1) | (1ULL << 7) | (phys & 0x000000FFFFE00000ULL);

                asm volatile("invlpg (%0)" : : "r"((void *)v_addr) : "memory");
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

                void *virt = getVirtAddrFromPageIndex(page_index + i);
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

            uint64_t start_index = getPageIndexFromVirtAddr(&kernel_virtual_end);
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
            return getVirtAddrFromPageIndex(start_index);
        }

        int freePage(const void *addr, uint64_t num_pages)
        {
            unmapPages(getPageIndexFromVirtAddr(addr), num_pages);
            return 0;
        }
    }; // namespace VirtualMemory
}; // namespace Memory