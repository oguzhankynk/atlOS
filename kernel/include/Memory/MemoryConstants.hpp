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

    static size_t ROUND_UP_TO_PAGE_SIZE(size_t size)
    {
        // If size is already a multiple of PAGE_SIZE, return it as is
        // Otherwise, round up to the next multiple of 4096
        return (size % PAGE_SIZE == 0) ? size : ((size / PAGE_SIZE) + 1) * PAGE_SIZE;
    }
};

#endif /* E3B89885_6058_4A97_A247_B94C13DBD57E */
