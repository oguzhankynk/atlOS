#ifndef E3B89885_6058_4A97_A247_B94C13DBD57E
#define E3B89885_6058_4A97_A247_B94C13DBD57E

#include <stdint.h>

namespace Memory
{
    /**
     * @brief Size of a single memory page.
     *
     */
    constexpr uint32_t PAGE_SIZE = 4096;

    /**
     * @brief Number of entries per page directory.
     */
    constexpr uint32_t PAGE_DIR_SIZE = 1024;

    /**
     * @brief Number of entries per page table.
     */
    constexpr uint32_t PAGE_TABLE_SIZE = 1024;

    constexpr uint32_t KERNEL_LOAD_ADDR = (uint32_t)0xC0000000;
};

#endif /* E3B89885_6058_4A97_A247_B94C13DBD57E */
