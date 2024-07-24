#include <Memory/VirtualMemory.hpp>

#include <string.h>
#include <stdio.h>
#include <kernel/panic.h>

#include <Memory/Memory.hpp>
#include <Memory/MemoryConstants.hpp>
#include <Memory/PhysicalMemory.hpp>


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

        static uint32_t FindContiguousFreePages(size_t num_pages);
        static uint32_t UnmapPages(uint32_t page_index, uint32_t num_pages);

        static void loadPageDirectory(uint32_t *page_directory);
        static void enablePaging();

        static inline uint32_t PAGE_INDEX(const void *virt_addr)
        {
            return (uint32_t)virt_addr / PAGE_SIZE;
        }

        static inline void *PAGE_INDEX_TO_VIRTADDR(uint32_t index)
        {
            return (void*)((index) * PAGE_SIZE);
        }

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

        void *getPageTableVirtAddr(uint32_t pdindex)
        {
            // because of page_directory[1023] set to page_directory self,
            // 0xFFC00000 points to page_directory
            return (void *)(0xFFC00000 + (pdindex * PAGE_SIZE));
        }

        

        uint32_t MapPages(uint32_t page_index, uint32_t num_pages)
        {
            uint32_t pdindex = page_index / PAGE_TABLE_SIZE; 
            uint32_t ptindex = page_index % PAGE_TABLE_SIZE;

            PageTable *pt = (PageTable*)getPageTableVirtAddr(pdindex);

            void *physicalFramesStart = PhysicalMemory::AllocateBlock(num_pages);

            if(physicalFramesStart == nullptr)
            {
                kernel_panic("VirtualMemory::MapPages not enough physical frame \n ");
            }

            uint32_t allocated{};
            void* physicalFrame = physicalFramesStart;

            while(allocated < num_pages)
            {
                PageTableEntry& pte = (*pt)[ptindex];

                if(pte.present)
                {
                    printf("page_index: 0x%x num_pages: 0x%x pdindex: 0x%x ptindex: 0x%x pte: 0x%x \n",
                     page_index, num_pages, pdindex, ptindex, pte);
                    kernel_panic("VirtualMemory::MapPages page already mapped \n ");
                }

                (*pt)[ptindex] = (uint32_t)physicalFrame | 3; //present , rw

                void *virtaddr = PAGE_INDEX_TO_VIRTADDR(pdindex*PAGE_TABLE_SIZE + ptindex);

                asm volatile("invlpg (%0)" : : "a"(virtaddr));

                allocated++;

                physicalFrame += PAGE_SIZE;

                ptindex++;

                if(ptindex == PAGE_TABLE_SIZE)
                {
                    pdindex++;
                    if(pdindex >= PAGE_DIR_SIZE)
                    {
                        kernel_panic("VirtualMemory::MapPages end of page directory \n");
                    }

                    ptindex=0;

                    pt = (PageTable*)getPageTableVirtAddr(pdindex);
                }
            }
            return allocated;
        }

        static uint32_t UnmapPages(uint32_t page_index, uint32_t num_pages)
        {
            uint32_t pdindex = page_index / PAGE_TABLE_SIZE; 
            uint32_t ptindex = page_index % PAGE_TABLE_SIZE;

            PageTable *pt = (PageTable*)getPageTableVirtAddr(pdindex);

            uint32_t deallocated{};

            while(deallocated < num_pages)
            {
                PageTableEntry& pte = (*pt)[ptindex];

                if( !pte.present )
                {
                    kernel_panic("VirtualMemory::UnmapPages page not present \n");
                }

                PhysicalMemory::FreeBlocks((void*)pte.frame, 1);
                memset(&pte, 0, sizeof(PageTableEntry));

                void *virtaddr = PAGE_INDEX_TO_VIRTADDR(pdindex*PAGE_TABLE_SIZE + ptindex);

                asm volatile("invlpg (%0)" : : "a"(virtaddr));

                deallocated++;

                ptindex++;

                if(ptindex == PAGE_TABLE_SIZE)
                {
                    pdindex++;
                    if(pdindex >= PAGE_DIR_SIZE)
                    {
                        kernel_panic("VirtualMemory::MapPages end of page directory \n");
                    }

                    ptindex=0;

                    pt = (PageTable*)getPageTableVirtAddr(pdindex);
                }
            }

            return deallocated;
        }

        void *AllocatePage(uint32_t num_pages)
        {
            uint32_t PageIndexStart =  FindContiguousFreePages(num_pages);

            uint32_t AllocatedPageNum = MapPages(PageIndexStart, num_pages);
            if(AllocatedPageNum != num_pages)
            {
                kernel_panic("VirtualMemory::AllocatePage error \n");
            }
            return PAGE_INDEX_TO_VIRTADDR(PageIndexStart);
        }

        int FreePage(const void *addr, uint32_t num_pages)
        {
            auto deallocated = UnmapPages(PAGE_INDEX(addr), num_pages);

            if(deallocated != num_pages)
            {
                printf("VirtualMemory::FreePage error \n");
                return -1;
            }

            return 0;
        }

        static uint32_t FindContiguousFreePages(size_t num_pages)
        {
            uint32_t start_pdindex = PAGE_INDEX((void *)KERNEL_LOAD_ADDR) / PAGE_TABLE_SIZE;
            uint32_t end_pdindex = PAGE_INDEX(&_kernel_end) / PAGE_TABLE_SIZE;

            uint32_t counter{};

            for (uint32_t pdindex = end_pdindex; pdindex < PAGE_DIR_SIZE; pdindex++)
            {
                if (!page_directory[pdindex].present)
                {
                    if (counter + PAGE_TABLE_SIZE >= num_pages)
                    {
                        return pdindex * PAGE_TABLE_SIZE - counter;
                    }

                    counter += PAGE_TABLE_SIZE;
                    continue;
                }

                for (uint32_t ptindex = 0; ptindex < PAGE_TABLE_SIZE; ptindex++)
                {
                    PageTableEntry& pte = ((PageTableEntry*)getPageTableVirtAddr(pdindex))[ptindex];

                    if(pte.present)
                    {
                        counter = 0;
                    }
                    else
                    {
                        counter++;
                        if(counter == num_pages)
                        {
                            return pdindex * PAGE_TABLE_SIZE + ptindex - counter +1;
                        }
                    }
                }
            }

            kernel_panic("FindContiguousFreePages error: NOMEM \n");
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
        static void loadPageDirectory(uint32_t *page_directory)
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
        static void enablePaging()
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