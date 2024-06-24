#ifndef AA43D675_0DC2_4B6E_8A42_68A48886962B
#define AA43D675_0DC2_4B6E_8A42_68A48886962B

#include <stdint.h>

/**
 * @brief Page Directory Entry (PDE)
 *
 * A single entry in a page directory. It contains the physical address of a page table
 * and various control bits.
 */
struct PageDirectory
{
    uint32_t present : 1;     /**< Page table present in memory */
    uint32_t rw : 1;          /**< Read/write permission */
    uint32_t user : 1;        /**< User/supervisor permission */
    uint32_t pwt : 1;         /**< Page-level write-through */
    uint32_t pcd : 1;         /**< Page-level cache disable */
    uint32_t accessed : 1;    /**< Page table accessed */
    uint32_t reserved : 1;    /**< Reserved bit */
    uint32_t page_size : 1;   /**< Page size (0 = 4 KB, 1 = 4 MB) */
    uint32_t ignored : 4;     /**< Ignored bits */
    uint32_t table_addr : 20; /**< Physical address of the page table (shifted right 12 bits) */
};

/**
 * @brief Page Table Entry (PTE)
 *
 * A single entry in a page table. It contains the physical address of the page
 * and various control bits.
 */
struct PageTableEntry
{
    uint32_t present : 1;  /**< Page present in memory */
    uint32_t rw : 1;       /**< Read/write permission */
    uint32_t user : 1;     /**< User/supervisor permission */
    uint32_t pwt : 1;      /**< Page-level write-through */
    uint32_t pcd : 1;      /**< Page-level cache disable */
    uint32_t accessed : 1; /**< Page accessed */
    uint32_t dirty : 1;    /**< Page written to */
    uint32_t pat : 1;      /**< Page attribute table */
    uint32_t global : 1;   /**< Global page */
    uint32_t ignored : 3;  /**< Ignored bits */
    uint32_t frame : 20;   /**< Physical frame address (shifted right 12 bits) */
};

namespace Memory
{
    static constexpr uint32_t KERNEL_START = 0xC0000000; /**< the start address of the kernel in virtual memory*/

    static constexpr uint32_t PAGE_DIR_SIZE = 1024; /**< Entries per page directory */
    static constexpr uint32_t PAGE_TABLE_SIZE = 1024; /**< Entries per page table */

    void initMemory(uint32_t memHigh, uint32_t physicalAllocStart);

} // namespace name

#endif /* AA43D675_0DC2_4B6E_8A42_68A48886962B */
