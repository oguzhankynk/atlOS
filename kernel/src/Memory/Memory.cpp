#include <Memory/Memory.hpp>
#include <kernel/sys/multiboot.h>
#include <stdio.h>
#include <string.h>

namespace Memory
{

    /**
     * @brief Number of entries per page directory.
     */
    static constexpr uint32_t PAGE_DIR_SIZE = 1024;

    /**
     * @brief Number of entries per page table.
     */
    static constexpr uint32_t PAGE_TABLE_SIZE = 1024; /**< Entries per page table */

    /**
     * @brief Page directory aligned to a 4096-byte boundary.
     *
     */
    PageDirectory page_directory[PAGE_DIR_SIZE] __attribute__((aligned(4096)));

    /**
     * @brief First page table aligned to a 4096-byte boundary.
     *
     */
    PageTableEntry first_page_table[PAGE_TABLE_SIZE] __attribute__((aligned(4096)));

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

    void loadPageDirectory(uint32_t *page_directory);
    void enablePaging();
    void *get_physaddr(void *virtualaddr);

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

    void initMemory(uint32_t memHigh, uint32_t physicalAllocStart)
    {
        memset(first_page_table, 0, sizeof(first_page_table));

        for (int i = 0; i < 1024; i++)
        {
            // Supervisor:
            // Write Enabled:
            // Not Present:
            page_directory[i].present = 0;
            page_directory[i].rw = 1;
            page_directory[i].user = 0;
            page_directory[i].pwt = 0;
            page_directory[i].pcd = 0;
            page_directory[i].accessed = 0;
            page_directory[i].reserved = 0;
            page_directory[i].page_size = 0;
            page_directory[i].ignored = 0;
            page_directory[i].table_addr = 0;
        }

        for (int i = 0; i < 1024; i++)
        {
            // supervisor level, read/write, present.

            first_page_table[i].present = 1;
            first_page_table[i].rw = 1;
            first_page_table[i].user = 0;
            first_page_table[i].pwt = 0;
            first_page_table[i].pcd = 0;
            first_page_table[i].accessed = 0;
            first_page_table[i].dirty = 0;
            first_page_table[i].pat = 0;
            first_page_table[i].global = 0;
            first_page_table[i].ignored = 0;
            first_page_table[i].frame = (i * PAGE_SIZE) >> 12;
        }

        // attributes: supervisor level, read/write, present
        page_directory[0].present = 1;
        page_directory[0].rw = 1;
        page_directory[0].user = 0;
        page_directory[0].pwt = 0;
        page_directory[0].pcd = 0;
        page_directory[0].accessed = 0;
        page_directory[0].reserved = 0;
        page_directory[0].page_size = 0;
        page_directory[0].ignored = 0;
        page_directory[0].table_addr = (uint32_t)first_page_table >> 12;

        printf("Paging will be enabled \n");

        // And this inside a function
        loadPageDirectory((unsigned int *)page_directory);
        enablePaging();

        printf("Paging  enabled \n");
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
        unsigned long pdindex = (unsigned long)virtualaddr >> 22;
        unsigned long ptindex = (unsigned long)virtualaddr >> 12 & 0x03FF;

        unsigned long *pd = (unsigned long *)0xFFFFF000;
        // Here you need to check whether the PD entry is present.

        unsigned long *pt = ((unsigned long *)0xFFC00000) + (0x400 * pdindex);
        // Here you need to check whether the PT entry is present.

        return (void *)((pt[ptindex] & ~0xFFF) + ((unsigned long)virtualaddr & 0xFFF));
    }

} // namespace Memory
