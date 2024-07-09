#include <Memory/VirtualMemory.hpp>

#include <string.h>

#include <Memory/Memory.hpp>
#include <Memory/MemoryConstants.hpp>


namespace Memory
{
    namespace VirtualMemory
    {
        /**
         * @brief Page directory defined at boot.S
         *
         */
        extern "C" PageDirectoryEntry page_directory[PAGE_DIR_SIZE];

        /**
         * @brief First page table defined at boot.S
         *
         */
        extern "C" PageTableEntry first_page_table[PAGE_TABLE_SIZE];

        PageTableEntry temp_page_table[PAGE_TABLE_SIZE] __attribute__((aligned(4096)));

        void loadPageDirectory(uint32_t *page_directory);
        void enablePaging();

        void Init()
        {
            memset(temp_page_table, 0, sizeof(temp_page_table));

            for (uint32_t addr = 0, i = 0; addr < 1024 * 1024; addr += PAGE_SIZE, i++)
            {
                temp_page_table[i] = addr | 3;
            }

            page_directory[0] = (uint32_t)Memory::VirtToPhysAddr((void *)temp_page_table) | 3;

        }

        /**
         * @brief Construct a new Page PageDirectoryEntry object
         *
         * @param value
         */
        PageDirectoryEntry::PageDirectoryEntry(uint32_t value)
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
    }; // namespace VirtualMemory
}; // namespace Memory