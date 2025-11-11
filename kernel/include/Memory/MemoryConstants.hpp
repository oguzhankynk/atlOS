#ifndef E3B89885_6058_4A97_A247_B94C13DBD57E
#define E3B89885_6058_4A97_A247_B94C13DBD57E

#include <stdint.h>

namespace Memory
{
    /**
     * @brief Size of a single memory page.
     *
     */
    constexpr uint32_t PAGE_SIZE = 2 * 1024 * 1024;

    /**
     * @brief Number of entries per page directory.
     */
    constexpr uint32_t PAGE_DIR_SIZE = 1024;

    /**
     * @brief Number of entries per page table.
     */
    constexpr uint32_t PAGE_TABLE_SIZE = 1024;

    constexpr uint32_t KERNEL_LOAD_ADDR = (uint32_t)0xC0000000;

    extern "C"
    {
        extern uint8_t _boottext_start;
        extern uint8_t _boottext_end;
        extern uint8_t _bootrodata_start;
        extern uint8_t _bootrodata_end;
        extern uint8_t _bootdata_start;
        extern uint8_t _bootdata_end;
        extern uint8_t _bootbss_start;
        extern uint8_t _bootbss_end;

        extern uint8_t _text_start;
        extern uint8_t _text_end;
        extern uint8_t _rodata_start;
        extern uint8_t _rodata_end;
        extern uint8_t _data_start;
        extern uint8_t _data_end;
        extern uint8_t _bss_start;
        extern uint8_t _bss_end;

        extern uint8_t __init_array_start;
        extern uint8_t __init_array_end;
        extern uint8_t __fini_array_start;
        extern uint8_t __fini_array_end;

        extern void (*__ctors_start[])(void);
        extern void (*__ctors_end[])(void);
        extern void (*__dtors_start[])(void);
        extern void (*__dtors_end[])(void);
    }

    /**
     * @brief Start address of the kernel.
     *
     * This variable is defined at linker.ld and represents the start address of
     * the kernel in virtual memory.
     */
    extern "C" uint32_t kernel_virtual_start;

    /**
     * @brief End address of the kernel.
     *
     * This variable is defined at linker.ld and represents the end address of
     * the kernel in virtual memory.
     */
    extern "C" uint32_t kernel_virtual_end;

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
