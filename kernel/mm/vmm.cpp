#include <mm/vmm.hpp>

#include <string.h>

#include <arch/x86_64/mmu/mmu.inc>
#include <core/logger.hpp>
#include <core/panic.hpp>
#include <mm/constants.hpp>
#include <mm/memory.hpp>
#include <mm/pmm.hpp>

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

namespace Memory {
/// Enable / Disable logs
static constexpr LogFlag dbgLog = LogFlag::Log;
static constexpr LogFlag infLog = LogFlag::Log;
static constexpr LogFlag errLog = LogFlag::Log;

/// A 2 MB page entry in a page directory
struct PageDirectoryEntry64 {
    uint64_t present : 1; /**< Page table present in memory */
    uint64_t rw : 1; /**< Read/write permission */
    uint64_t user : 1; /**< User/supervisor permission */
    uint64_t pwt : 1; /**< Page-level write-through */
    uint64_t pcd : 1; /**< Page-level cache disable */
    uint64_t accessed : 1; /**< Page table accessed */
    uint64_t dirty : 1; /**< Dirty; indicates whether software has written to the 2-MByte page referenced by this entry */
    uint64_t pageSize : 1; /**<  Page size; must be 1 */
    uint64_t global : 1; /**< Global; if CR4.PGE = 1, determines whether the translation is global (see Section 4.10); ignored otherwise */
    uint64_t ignored0 : 3; /**< Ignored bits */
    uint64_t reserved0 : 9; /**< Reserved bits */
    uint64_t frameAddr : 31; /**< 2MB aligned >> 21 */
    uint64_t reserved1 : 11; /**< Reserved bits */
    uint64_t nx : 1; /**< No-execute */
};

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
constexpr uint64_t recursiveSlot = 510;

// ------------------------------------------------------------
// CR3 helpers
// ------------------------------------------------------------

static inline uint64_t readCr3()
{
    uint64_t v;
    asm volatile("mov %%cr3, %0"
                 : "=r"(v));
    return v;
}

static inline void writeCr3(uint64_t v)
{
    asm volatile("mov %0, %%cr3" ::"r"(v)
                 : "memory");
}

constexpr uint64_t recursivePrefix = 0xFFFF000000000000ULL;

// ------------------------------------------------------------
// Recursive mapping virtual address helpers
// ------------------------------------------------------------

/**
 * @brief Virtual address of the PML4 itself via recursive mapping
 *
 * VA bits:
 *   [510][510][510][0]
 */
inline PageDirectoryEntry64* getPML4Virt()
{
    return (PageDirectoryEntry64*)(recursivePrefix | (recursiveSlot << 39) | (recursiveSlot << 30) | (recursiveSlot << 21) | (recursiveSlot << 12));
}

/**
 * @brief Get PDPT virtual address for a given PML4 index
 *
 * VA bits:
 *   [510][pml4_index][510][0]
 */
inline PageDirectoryEntry64* getPDPTVirt(uint64_t pml4Index)
{
    return (PageDirectoryEntry64*)(recursivePrefix | (recursiveSlot << 39) | (recursiveSlot << 30) | (recursiveSlot << 21) | (pml4Index << 12));
}

/**
 * @brief Get PD virtual address for a given PML4 + PDPT index
 *
 * VA bits:
 *   [510][pml4_index][pdpt_index][0]
 *
 * This is the FINAL level for 2 MB pages.
 */
inline PageDirectoryEntry64* getPDVirt(uint64_t pml4Index, uint64_t pdptIndex)
{
    return (PageDirectoryEntry64*)(recursivePrefix | (recursiveSlot << 39) | (recursiveSlot << 30) | (pml4Index << 21) | (pdptIndex << 12));
}

// ------------------------------------------------------------
// Page index helpers (2 MB pages)
// ------------------------------------------------------------

static inline uint64_t getPageIndexFromVirtAddr(const void* virtAddr)
{
    uintptr_t addr = (uintptr_t)virtAddr;
    if (addr < KERNEL_VIRTUAL_LOAD_OFFSET)
        kernelPanic("VMM: invalid kernel virtual address");

    return (addr - KERNEL_VIRTUAL_LOAD_OFFSET) >> 21;
}

static inline void* getVirtAddrFromPageIndex(uint64_t index)
{
    return (void*)(KERNEL_VIRTUAL_LOAD_OFFSET + (index << 21));
}

// ------------------------------------------------------------
// Raw entry helpers
//
// PageDirectoryEntry64 describes a 2 MB page entry only (frame_addr
// starts at bit 21). PML4/PDPT entries point to 4 KB tables, so they
// are written as raw 64-bit values.
// ------------------------------------------------------------

/// Physical address bits of an entry pointing to a 4 KB table (51..12)
constexpr uint64_t tableAddrMask = 0x000FFFFFFFFFF000ULL;

/// Physical address bits of a 2 MB page entry (51..21)
constexpr uint64_t page2MbAddrMask = 0x000FFFFFFFE00000ULL;

/// 2 MB pages from KERNEL_VIRTUAL_LOAD_OFFSET to the top of the address space (2 GB)
constexpr uint64_t maxPageIndex = (0ULL - KERNEL_VIRTUAL_LOAD_OFFSET) / pageSize2Mb;

/**
 * @brief Get the PD entry mapping a virtual address, via recursive mapping
 *
 * @return nullptr if the PML4 or PDPT entry is not present (no PD exists yet)
 */
static uint64_t* getPDEntry(uintptr_t vAddr)
{
    uint64_t pml4Idx = (vAddr >> 39) & 0x1FF;
    uint64_t pdptIdx = (vAddr >> 30) & 0x1FF;
    uint64_t pdIdx = (vAddr >> 21) & 0x1FF;

    uint64_t* pml4Table = (uint64_t*)getPML4Virt();
    if (!(pml4Table[pml4Idx] & MMU_PRESENT))
        return nullptr;

    uint64_t* pdptTable = (uint64_t*)getPDPTVirt(pml4Idx);
    if (!(pdptTable[pdptIdx] & MMU_PRESENT))
        return nullptr;

    return (uint64_t*)getPDVirt(pml4Idx, pdptIdx) + pdIdx;
}

// ------------------------------------------------------------
// Initialization
// ------------------------------------------------------------

AddressSpace::AddressSpace(PhysicalMemoryManager& pmm)
    : m_pmm(pmm)
{
    logMsg(infLog, "VMM: init start. Setting up recursive mapping...\n");

    uint64_t currentCr3 = readCr3() & ~0xFFFULL;

    uint64_t* pml4Raw = (uint64_t*)&pml4[0];
    pml4Raw[recursiveSlot] = currentCr3 | 0x03;

    // Full TLB flush is REQUIRED here
    writeCr3(readCr3());

    logMsg(infLog, "VMM: Recursive mapping set at PML4[%llu]: 0x%llx\n", recursiveSlot, *(uint64_t*)&pml4[recursiveSlot]);
    logMsg(infLog, "VMM: init done.\n");
}

/**
 * @brief Get the PD entry mapping a virtual address, creating the PD if needed
 */
uint64_t* AddressSpace::getOrCreatePdEntry(uintptr_t vAddr)
{
    uint64_t pml4Idx = (vAddr >> 39) & 0x1FF;
    uint64_t pdptIdx = (vAddr >> 30) & 0x1FF;
    uint64_t pdIdx = (vAddr >> 21) & 0x1FF;

    uint64_t* pml4Table = (uint64_t*)getPML4Virt();

    // PDPT check (PML4 entry)
    if (!(pml4Table[pml4Idx] & MMU_PRESENT)) {
        kernelPanic("Dynamic PDPT creation not implemented yet!");
    }

    uint64_t* pdptTable = (uint64_t*)getPDPTVirt(pml4Idx);

    if (!(pdptTable[pdptIdx] & MMU_PRESENT)) {
        uint64_t newPdPhys = (uintptr_t)m_pmm.allocateFrame();
        if (newPdPhys == 0)
            kernelPanic("VMM: out of physical memory for page directory");

        pdptTable[pdptIdx] = (newPdPhys & tableAddrMask) | MMU_PRESENT | MMU_WRITABLE;

        PageDirectoryEntry64* pdTable = getPDVirt(pml4Idx, pdptIdx);
        asm volatile("invlpg (%0)"
                     :
                     : "r"(pdTable)
                     : "memory");

        memset(pdTable, 0, frameSize4Kb);
    }

    return (uint64_t*)getPDVirt(pml4Idx, pdptIdx) + pdIdx;
}

/**
 * @brief Clear a PD entry and give the PD back to the PMM once it is empty
 *
 * The boot PDs live in the kernel image, they are never freed.
 */
void AddressSpace::clearPdEntry(uintptr_t vAddr, uint64_t* pdEntry)
{
    *pdEntry = 0;
    asm volatile("invlpg (%0)"
                 :
                 : "r"((void*)vAddr)
                 : "memory");

    uint64_t* pdTable = (uint64_t*)((uintptr_t)pdEntry & ~(uintptr_t)(frameSize4Kb - 1));
    for (uint64_t i = 0; i < 512; i++) {
        if (pdTable[i] & MMU_PRESENT)
            return;
    }

    uint64_t pml4Idx = (vAddr >> 39) & 0x1FF;
    uint64_t pdptIdx = (vAddr >> 30) & 0x1FF;
    uint64_t* pdptTable = (uint64_t*)getPDPTVirt(pml4Idx);
    uint64_t pdPhys = pdptTable[pdptIdx] & tableAddrMask;

    if (pdPhys == (uintptr_t)high_page_directory_table || pdPhys == (uintptr_t)low_page_directory_table)
        return;

    pdptTable[pdptIdx] = 0;
    asm volatile("invlpg (%0)"
                 :
                 : "r"(pdTable)
                 : "memory");

    m_pmm.freeFrame((void*)pdPhys);
    logMsg(dbgLog, "VMM: freed empty PD at 0x%llx (PML4=%llu, PDPT=%llu)\n", pdPhys, pml4Idx, pdptIdx);
}

void AddressSpace::mapPages(uint64_t pageIndex, uint64_t count)
{
    for (uint64_t i = 0; i < count; ++i) {
        uintptr_t vAddr = (uintptr_t)getVirtAddrFromPageIndex(pageIndex + i);

        logMsg(dbgLog, "VMM: Mapping page %llu: VA=0x%llx\n", pageIndex + i, vAddr);

        uint64_t* pdEntry = getOrCreatePdEntry(vAddr);
        if (*pdEntry & MMU_PRESENT)
            kernelPanic("VMM: mapPages: page already mapped!");

        uint64_t phys = (uintptr_t)m_pmm.allocateBlocks(1);
        if (phys == 0)
            kernelPanic("VMM: out of physical memory");

        *pdEntry = (phys & page2MbAddrMask) | MMU_PRESENT | MMU_WRITABLE | MMU_PDE_TWO_MB;

        asm volatile("invlpg (%0)"
                     :
                     : "r"((void*)vAddr)
                     : "memory");
    }
}

void AddressSpace::unmapPages(uint64_t pageIndex, uint64_t count)
{
    for (uint64_t i = 0; i < count; ++i) {
        uintptr_t vAddr = (uintptr_t)getVirtAddrFromPageIndex(pageIndex + i);

        uint64_t* pdEntry = getPDEntry(vAddr);
        if (pdEntry == nullptr || !(*pdEntry & MMU_PRESENT))
            kernelPanic("VMM: unmapPages: page not present!");

        void* physAddr = (void*)(*pdEntry & page2MbAddrMask);
        m_pmm.freeBlocks(physAddr, 1);

        clearPdEntry(vAddr, pdEntry);
    }
}

void AddressSpace::mapPhysical(const void* virtAddr, const void* physAddr)
{
    uintptr_t vAddr = (uintptr_t)virtAddr;
    uint64_t phys = (uintptr_t)physAddr;

    if ((vAddr | phys) & (pageSize2Mb - 1))
        kernelPanic("VMM: mapPhysical: addresses must be 2 MB aligned");

    uint64_t* pdEntry = getOrCreatePdEntry(vAddr);
    if (*pdEntry & MMU_PRESENT)
        kernelPanic("VMM: mapPhysical: page already mapped!");

    *pdEntry = (phys & page2MbAddrMask) | MMU_PRESENT | MMU_WRITABLE | MMU_PDE_TWO_MB;

    asm volatile("invlpg (%0)"
                 :
                 : "r"((void*)vAddr)
                 : "memory");
}

void AddressSpace::unmapPhysical(const void* virtAddr)
{
    uintptr_t vAddr = (uintptr_t)virtAddr & ~(uintptr_t)(pageSize2Mb - 1);

    uint64_t* pdEntry = getPDEntry(vAddr);
    if (pdEntry == nullptr || !(*pdEntry & MMU_PRESENT))
        kernelPanic("VMM: unmapPhysical: page not present!");

    clearPdEntry(vAddr, pdEntry);
}

void* AddressSpace::virtToPhys(const void* virtAddr) const
{
    uintptr_t vAddr = (uintptr_t)virtAddr;
    uint64_t pml4Idx = (vAddr >> 39) & 0x1FF;
    uint64_t pdptIdx = (vAddr >> 30) & 0x1FF;

    uint64_t* pml4Table = (uint64_t*)getPML4Virt();
    if (!(pml4Table[pml4Idx] & MMU_PRESENT))
        return nullptr;

    // 1 GB page
    uint64_t pdptEntry = ((uint64_t*)getPDPTVirt(pml4Idx))[pdptIdx];
    if (!(pdptEntry & MMU_PRESENT))
        return nullptr;
    if (pdptEntry & MMU_PDE_TWO_MB)
        return (void*)((pdptEntry & 0x000FFFFFC0000000ULL) | (vAddr & 0x3FFFFFFFULL));

    uint64_t* pdEntry = getPDEntry(vAddr);
    if (!(*pdEntry & MMU_PRESENT))
        return nullptr;

    // Only 2 MB pages are used, there are no 4 KB page tables
    if (!(*pdEntry & MMU_PDE_TWO_MB))
        kernelPanic("VMM: virtToPhys: 4 KB pages are not supported");

    return (void*)((*pdEntry & page2MbAddrMask) | (vAddr & (pageSize2Mb - 1)));
}

/// Simple contiguous page allocator
static uint64_t findContiguousFreePages(uint64_t numPages)
{

    uint64_t startIndex = getPageIndexFromVirtAddr(&kernel_virtual_end);

    for (uint64_t i = startIndex; i + numPages <= maxPageIndex; ++i) {
        bool free = true;
        for (uint64_t j = 0; j < numPages; ++j) {
            // No PD yet means nothing in its 1 GB range is mapped
            uint64_t* pdEntry = getPDEntry((uintptr_t)getVirtAddrFromPageIndex(i + j));
            if (pdEntry != nullptr && (*pdEntry & MMU_PRESENT)) {
                free = false;
                i += j;
                break;
            }
        }
        if (free)
            return i;
    }

    kernelPanic("VMM: findContiguousFreePages: no free area!");
    return 0;
}

void* AddressSpace::allocatePages(uint64_t count)
{
    uint64_t startIndex = findContiguousFreePages(count);
    mapPages(startIndex, count);
    return getVirtAddrFromPageIndex(startIndex);
}

void AddressSpace::freePages(const void* base, uint64_t count)
{
    unmapPages(getPageIndexFromVirtAddr(base), count);
}
}; // namespace Memory