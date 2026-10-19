#ifndef A85ADD73_9A97_40E4_AB3B_BAB8F30BD473
#define A85ADD73_9A97_40E4_AB3B_BAB8F30BD473

#include <stdint.h>

#include <Memory/MemoryConstants.hpp>

namespace Memory
{
    namespace VirtualMemory
    {

        /**
         * @brief Page Directory Entry (PDE)
         *
         * For more details, see:  Intel® 64 and IA-32 Architectures Software Developer Manuals, volume 3A
         * Table 4-17. Format of an IA-32e Page-Directory Entry that Maps a 2-MByte Page
         */
        struct PageDirectoryEntry64
        {
            uint64_t present : 1;     /**< Page table present in memory */
            uint64_t rw : 1;          /**< Read/write permission */
            uint64_t user : 1;        /**< User/supervisor permission */
            uint64_t pwt : 1;         /**< Page-level write-through */
            uint64_t pcd : 1;         /**< Page-level cache disable */
            uint64_t accessed : 1;    /**< Page table accessed */
            uint64_t dirty : 1;       /**< Dirty; indicates whether software has written to the 2-MByte page referenced by this entry */
            uint64_t page_size : 1;   /**<  Page size; must be 1 */
            uint64_t global : 1;      /**< Global; if CR4.PGE = 1, determines whether the translation is global (see Section 4.10); ignored otherwise */
            uint64_t ignored0 : 3;    /**< Ignored bits */
            uint64_t reserved0 : 9;   /**< Reserved bits */
            uint64_t frame_addr : 31; /**< 2MB aligned >> 21 */
            uint64_t reserved1 : 11;  /**< Reserved bits */
            uint64_t nx : 1;          /**< No-execute */
        };

        

        void init();
        uint64_t mapPages(uint64_t page_index, uint64_t num_pages);
        void *allocatePage(uint64_t num_pages);
        int freePage(const void *addr, uint64_t num_pages);

        /**
         * @brief Map a 2 MB page to a given physical address (e.g. for MMIO).
         * The physical memory is not taken from or returned to the PMM.
         */
        void mapPhysical(const void *virt_addr, const void *phys_addr);

        /**
         * @brief Remove a mapping made by mapPhysical. The physical memory is not freed.
         */
        void unmapPhysical(const void *virt_addr);

        /**
         * @brief Translate a virtual address by walking the page tables.
         *
         * @return The physical address, or nullptr if it is not mapped.
         */
        void *virtToPhys(const void *virt_addr);

    }; // namespace VirtualMemory
}; // namespace Memory
#endif /* A85ADD73_9A97_40E4_AB3B_BAB8F30BD473 */
