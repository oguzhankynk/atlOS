#include <mm/vmm.hpp>

#include <string.h>

#include <core/panic.hpp>
#include <core/logger.hpp>
#include <mm/memory.hpp>
#include <mm/constants.hpp>
#include <mm/pmm.hpp>
#include <arch/x86_64/mmu/mmu.inc>

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

        // ------------------------------------------------------------
        // Raw entry helpers
        //
        // PageDirectoryEntry64 describes a 2 MB page entry only (frame_addr
        // starts at bit 21). PML4/PDPT entries point to 4 KB tables, so they
        // are written as raw 64-bit values.
        // ------------------------------------------------------------

        /// Physical address bits of an entry pointing to a 4 KB table (51..12)
        constexpr uint64_t TABLE_ADDR_MASK = 0x000FFFFFFFFFF000ULL;

        /// Physical address bits of a 2 MB page entry (51..21)
        constexpr uint64_t PAGE_2MB_ADDR_MASK = 0x000FFFFFFFE00000ULL;

        /// 2 MB pages from KERNEL_VIRTUAL_LOAD_OFFSET to the top of the address space (2 GB)
        constexpr uint64_t MAX_PAGE_INDEX = (0ULL - KERNEL_VIRTUAL_LOAD_OFFSET) / PAGE_SIZE_2MB;

        /**
         * @brief Get the PD entry mapping a virtual address, via recursive mapping
         *
         * @return nullptr if the PML4 or PDPT entry is not present (no PD exists yet)
         */
        static uint64_t *getPDEntry(uintptr_t v_addr)
        {
            uint64_t pml4_idx = (v_addr >> 39) & 0x1FF;
            uint64_t pdpt_idx = (v_addr >> 30) & 0x1FF;
            uint64_t pd_idx = (v_addr >> 21) & 0x1FF;

            uint64_t *pml4_table = (uint64_t *)getPML4Virt();
            if (!(pml4_table[pml4_idx] & MMU_PRESENT))
                return nullptr;

            uint64_t *pdpt_table = (uint64_t *)getPDPTVirt(pml4_idx);
            if (!(pdpt_table[pdpt_idx] & MMU_PRESENT))
                return nullptr;

            return (uint64_t *)getPDVirt(pml4_idx, pdpt_idx) + pd_idx;
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
        /**
         * @brief Get the PD entry mapping a virtual address, creating the PD if needed
         */
        static uint64_t *getOrCreatePDEntry(uintptr_t v_addr)
        {
            uint64_t pml4_idx = (v_addr >> 39) & 0x1FF;
            uint64_t pdpt_idx = (v_addr >> 30) & 0x1FF;
            uint64_t pd_idx = (v_addr >> 21) & 0x1FF;

            uint64_t *pml4_table = (uint64_t *)getPML4Virt();

            // PDPT check (PML4 entry)
            if (!(pml4_table[pml4_idx] & MMU_PRESENT))
            {
                kernel_panic("Dynamic PDPT creation not implemented yet!");
            }

            uint64_t *pdpt_table = (uint64_t *)getPDPTVirt(pml4_idx);

            if (!(pdpt_table[pdpt_idx] & MMU_PRESENT))
            {
                uint64_t new_pd_phys = (uintptr_t)PhysicalMemory::AllocateFrame();
                if (new_pd_phys == 0)
                    kernel_panic("VMM: out of physical memory for page directory");

                pdpt_table[pdpt_idx] = (new_pd_phys & TABLE_ADDR_MASK) | MMU_PRESENT | MMU_WRITABLE;

                PageDirectoryEntry64 *pd_table = getPDVirt(pml4_idx, pdpt_idx);
                asm volatile("invlpg (%0)" : : "r"(pd_table) : "memory");

                memset(pd_table, 0, FRAME_SIZE_4KB);
            }

            return (uint64_t *)getPDVirt(pml4_idx, pdpt_idx) + pd_idx;
        }

        /**
         * @brief Clear a PD entry and give the PD back to the PMM once it is empty
         *
         * The boot PDs live in the kernel image, they are never freed.
         */
        static void clearPDEntry(uintptr_t v_addr, uint64_t *pd_entry)
        {
            *pd_entry = 0;
            asm volatile("invlpg (%0)" : : "r"((void *)v_addr) : "memory");

            uint64_t *pd_table = (uint64_t *)((uintptr_t)pd_entry & ~(uintptr_t)(FRAME_SIZE_4KB - 1));
            for (uint64_t i = 0; i < 512; i++)
            {
                if (pd_table[i] & MMU_PRESENT)
                    return;
            }

            uint64_t pml4_idx = (v_addr >> 39) & 0x1FF;
            uint64_t pdpt_idx = (v_addr >> 30) & 0x1FF;
            uint64_t *pdpt_table = (uint64_t *)getPDPTVirt(pml4_idx);
            uint64_t pd_phys = pdpt_table[pdpt_idx] & TABLE_ADDR_MASK;

            if (pd_phys == (uintptr_t)high_page_directory_table || pd_phys == (uintptr_t)low_page_directory_table)
                return;

            pdpt_table[pdpt_idx] = 0;
            asm volatile("invlpg (%0)" : : "r"(pd_table) : "memory");

            PhysicalMemory::FreeFrame((void *)pd_phys);
            LOG_MSG(DBG_LOG, "VMM: freed empty PD at 0x%llx (PML4=%llu, PDPT=%llu)\n", pd_phys, pml4_idx, pdpt_idx);
        }

        uint64_t mapPages(uint64_t page_index, uint64_t num_pages)
        {
            for (uint64_t i = 0; i < num_pages; ++i)
            {
                uintptr_t v_addr = (uintptr_t)getVirtAddrFromPageIndex(page_index + i);

                LOG_MSG(DBG_LOG, "VMM: Mapping page %llu: VA=0x%llx\n", page_index + i, v_addr);

                uint64_t *pd_entry = getOrCreatePDEntry(v_addr);
                if (*pd_entry & MMU_PRESENT)
                    kernel_panic("VMM: mapPages: page already mapped!");

                uint64_t phys = (uintptr_t)PhysicalMemory::AllocateBlock(1);
                if (phys == 0)
                    kernel_panic("VMM: out of physical memory");

                *pd_entry = (phys & PAGE_2MB_ADDR_MASK) | MMU_PRESENT | MMU_WRITABLE | MMU_PDE_TWO_MB;

                asm volatile("invlpg (%0)" : : "r"((void *)v_addr) : "memory");
            }
            return num_pages;
        }

        uint64_t unmapPages(uint64_t page_index, uint64_t num_pages)
        {
            for (uint64_t i = 0; i < num_pages; ++i)
            {
                uintptr_t v_addr = (uintptr_t)getVirtAddrFromPageIndex(page_index + i);

                uint64_t *pd_entry = getPDEntry(v_addr);
                if (pd_entry == nullptr || !(*pd_entry & MMU_PRESENT))
                    kernel_panic("VMM: unmapPages: page not present!");

                void *phys_addr = (void *)(*pd_entry & PAGE_2MB_ADDR_MASK);
                PhysicalMemory::FreeBlocks(phys_addr, 1);

                clearPDEntry(v_addr, pd_entry);
            }

            return num_pages;
        }

        void mapPhysical(const void *virt_addr, const void *phys_addr)
        {
            uintptr_t v_addr = (uintptr_t)virt_addr;
            uint64_t phys = (uintptr_t)phys_addr;

            if ((v_addr | phys) & (PAGE_SIZE_2MB - 1))
                kernel_panic("VMM: mapPhysical: addresses must be 2 MB aligned");

            uint64_t *pd_entry = getOrCreatePDEntry(v_addr);
            if (*pd_entry & MMU_PRESENT)
                kernel_panic("VMM: mapPhysical: page already mapped!");

            *pd_entry = (phys & PAGE_2MB_ADDR_MASK) | MMU_PRESENT | MMU_WRITABLE | MMU_PDE_TWO_MB;

            asm volatile("invlpg (%0)" : : "r"((void *)v_addr) : "memory");
        }

        void unmapPhysical(const void *virt_addr)
        {
            uintptr_t v_addr = (uintptr_t)virt_addr & ~(uintptr_t)(PAGE_SIZE_2MB - 1);

            uint64_t *pd_entry = getPDEntry(v_addr);
            if (pd_entry == nullptr || !(*pd_entry & MMU_PRESENT))
                kernel_panic("VMM: unmapPhysical: page not present!");

            clearPDEntry(v_addr, pd_entry);
        }

        void *virtToPhys(const void *virt_addr)
        {
            uintptr_t v_addr = (uintptr_t)virt_addr;
            uint64_t pml4_idx = (v_addr >> 39) & 0x1FF;
            uint64_t pdpt_idx = (v_addr >> 30) & 0x1FF;

            uint64_t *pml4_table = (uint64_t *)getPML4Virt();
            if (!(pml4_table[pml4_idx] & MMU_PRESENT))
                return nullptr;

            // 1 GB page
            uint64_t pdpt_entry = ((uint64_t *)getPDPTVirt(pml4_idx))[pdpt_idx];
            if (!(pdpt_entry & MMU_PRESENT))
                return nullptr;
            if (pdpt_entry & MMU_PDE_TWO_MB)
                return (void *)((pdpt_entry & 0x000FFFFFC0000000ULL) | (v_addr & 0x3FFFFFFFULL));

            uint64_t *pd_entry = getPDEntry(v_addr);
            if (!(*pd_entry & MMU_PRESENT))
                return nullptr;

            // Only 2 MB pages are used, there are no 4 KB page tables
            if (!(*pd_entry & MMU_PDE_TWO_MB))
                kernel_panic("VMM: virtToPhys: 4 KB pages are not supported");

            return (void *)((*pd_entry & PAGE_2MB_ADDR_MASK) | (v_addr & (PAGE_SIZE_2MB - 1)));
        }

        /// Simple contiguous page allocator
        static uint64_t findContiguousFreePages(uint64_t num_pages)
        {

            uint64_t start_index = getPageIndexFromVirtAddr(&kernel_virtual_end);

            for (uint64_t i = start_index; i + num_pages <= MAX_PAGE_INDEX; ++i)
            {
                bool free = true;
                for (uint64_t j = 0; j < num_pages; ++j)
                {
                    // No PD yet means nothing in its 1 GB range is mapped
                    uint64_t *pd_entry = getPDEntry((uintptr_t)getVirtAddrFromPageIndex(i + j));
                    if (pd_entry != nullptr && (*pd_entry & MMU_PRESENT))
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