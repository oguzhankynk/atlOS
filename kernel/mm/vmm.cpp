#include <mm/vmm.hpp>

#include <string.h>

#include <arch/x86_64/mmu/mmu.inc>
#include <core/logger.hpp>
#include <core/panic.hpp>
#include <mm/constants.hpp>
#include <mm/memory.hpp>
#include <mm/pmm.hpp>

/*
    x86_64 4-level paging with 2 MB pages (PML4 -> PDPT -> PD -> 2 MB page):

    63  . . . 48 | 47     39 | 38     30 | 29     21 | 20     0
    -----------------------------------------------------------
    Sign extend  |   PML4    |   PDPT    |    PD     | Page offset

    Layout of every address space:

    PML4[0]        identity map of the first megabytes (boot.S), kernel only
    PML4[1..255]   user programs (userBase .. userEnd)
    PML4[256]      physmap: all physical memory at physmapBase + phys
    PML4[511]      the kernel image and kernel pages (-2 GB .. top)

    Entries 0 and 256..511 point at the same tables in every address space.

    The page tables hold physical addresses; the kernel reads and writes
    them through the physmap. That is what lets an AddressSpace edit tables
    that are not loaded in CR3 (a recursive mapping, used before, can only
    reach the active ones).
*/

namespace Memory {

static constexpr LogFlag dbgLog = LogFlag::NoLog; // every mapping: very chatty
static constexpr LogFlag infLog = LogFlag::Log;

/// Page tables built in boot.S (in the boot sections: physical address == symbol address)
extern "C" uint64_t pml4[512];

/// Physical address bits of an entry pointing to a 4 KB table (51..12)
static constexpr uint64_t tableAddrMask = 0x000FFFFFFFFFF000ULL;

/// Physical address bits of a 2 MB page entry (51..21)
static constexpr uint64_t page2MbAddrMask = 0x000FFFFFFFE00000ULL;

/// Bit 9 is ignored by the CPU: atlOS marks pages whose block this address space owns
static constexpr uint64_t entryOwned = 1ULL << 9;

static constexpr uint64_t physmapSlot = 256;
static constexpr uint64_t kernelHalfSlot = 256; // PML4[256..511] are shared
static constexpr uint64_t gigabyte = 1ULL << 30;
static constexpr uint64_t physmapGigabytes = physmapSize / gigabyte;

// The physmap's own tables live in the kernel image: they must exist before
// the physmap does, and the kernel image is mapped already
alignas(4096) static uint64_t s_physmapPdpt[512];
alignas(4096) static uint64_t s_physmapPds[physmapGigabytes][512];

// ------------------------------------------------------------
// Helpers
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

/// A table, by the physical address in an entry pointing at it
static uint64_t* table(uint64_t entryOrPhys)
{
    return static_cast<uint64_t*>(physToVirt(entryOrPhys & tableAddrMask));
}

/// Physical address of something in the kernel image (linked at KERNEL_VIRTUAL_LOAD_OFFSET + phys)
static uint64_t imagePhys(const void* p)
{
    return (uint64_t)p - KERNEL_VIRTUAL_LOAD_OFFSET;
}

static uint64_t canonical(uint64_t address)
{
    // Bits 63..48 must copy bit 47
    return (address & (1ULL << 47)) ? (address | 0xFFFF000000000000ULL) : address;
}

static uint64_t index4(uint64_t v) { return (v >> 39) & 0x1FF; }
static uint64_t index3(uint64_t v) { return (v >> 30) & 0x1FF; }
static uint64_t index2(uint64_t v) { return (v >> 21) & 0x1FF; }

// ------------------------------------------------------------
// Construction
// ------------------------------------------------------------

AddressSpace::AddressSpace(PhysicalMemoryManager& pmm)
    : m_pmm(pmm)
    , m_pml4(readCr3() & tableAddrMask)
    , m_isKernel(true)
{
    // Frames for new tables must be reachable through the physmap
    if (pmm.capacity() * PhysicalMemoryManager::blockSize > physmapSize)
        kernelPanic("VMM: the PMM manages more memory than the physmap covers");

    // physmap: one PD per gigabyte, 2 MB pages, written through the kernel image mapping
    for (uint64_t gb = 0; gb < physmapGigabytes; gb++) {
        for (uint64_t i = 0; i < 512; i++)
            s_physmapPds[gb][i] = (gb * gigabyte + i * pageSize2Mb) | MMU_PRESENT | MMU_WRITABLE | MMU_PDE_TWO_MB;
        s_physmapPdpt[gb] = imagePhys(s_physmapPds[gb]) | MMU_PRESENT | MMU_WRITABLE;
    }
    pml4[physmapSlot] = imagePhys(s_physmapPdpt) | MMU_PRESENT | MMU_WRITABLE;
    writeCr3(readCr3()); // flush the TLB

    logMsg(infLog, "VMM: physmap: %llu GB of physical memory at 0x%llx\n", physmapGigabytes, physmapBase);
}

AddressSpace::AddressSpace(PhysicalMemoryManager& pmm, const AddressSpace& kernel)
    : m_pmm(pmm)
    , m_pml4(0)
    , m_isKernel(false)
{
    m_pml4 = newTable();
    if (!m_pml4)
        kernelPanic("VMM: out of physical memory for a PML4");

    // Share the identity mapping and the whole kernel half
    uint64_t* mine = table(m_pml4);
    const uint64_t* kernelPml4 = table(kernel.m_pml4);
    mine[0] = kernelPml4[0];
    for (uint64_t i = kernelHalfSlot; i < 512; i++)
        mine[i] = kernelPml4[i];
}

AddressSpace::~AddressSpace()
{
    if (m_isKernel)
        return;
    if (isActive())
        kernelPanic("VMM: destroying the active address space");

    // Only the user range is this address space's own: its owned pages, its PDs and PDPTs
    uint64_t* pml4Table = table(m_pml4);
    for (uint64_t i = index4(userBase); i < kernelHalfSlot; i++) {
        if (!(pml4Table[i] & MMU_PRESENT))
            continue;
        uint64_t* pdpt = table(pml4Table[i]);
        for (uint64_t j = 0; j < 512; j++) {
            if (!(pdpt[j] & MMU_PRESENT))
                continue;
            uint64_t* pd = table(pdpt[j]);
            for (uint64_t k = 0; k < 512; k++) {
                if ((pd[k] & MMU_PRESENT) && (pd[k] & entryOwned))
                    m_pmm.freeBlocks((void*)(pd[k] & page2MbAddrMask), 1);
            }
            m_pmm.freeFrame((void*)(pdpt[j] & tableAddrMask));
        }
        m_pmm.freeFrame((void*)(pml4Table[i] & tableAddrMask));
    }
    m_pmm.freeFrame((void*)m_pml4);
}

// ------------------------------------------------------------
// Tables
// ------------------------------------------------------------

/// A zeroed 4 KB frame for a page table, 0 when memory ran out
uint64_t AddressSpace::newTable()
{
    uint64_t frame = (uintptr_t)m_pmm.allocateFrame();
    if (frame)
        memset(physToVirt(frame), 0, frameSize4Kb);
    return frame;
}

/// Give a table back to the PMM, unless it is part of the kernel image (boot.S, the physmap)
void AddressSpace::freeTableIfOwned(uint64_t tablePhys)
{
    if (tablePhys < (uintptr_t)&kernel_physical_end)
        return;
    m_pmm.freeFrame((void*)tablePhys);
}

/**
 * @brief The PD entry for virtAddr. With create, missing PDPTs and PDs are
 * made (nullptr if memory ran out); without, nullptr if there is none.
 */
uint64_t* AddressSpace::pdEntry(uint64_t virtAddr, bool create)
{
    // Tables below a user range entry must allow ring 3; the PD entry decides
    uint64_t parentFlags = MMU_PRESENT | MMU_WRITABLE | (virtAddr < userEnd && virtAddr >= userBase ? MMU_USER_MEMORY : 0);

    uint64_t& pml4Entry = table(m_pml4)[index4(virtAddr)];
    if (!(pml4Entry & MMU_PRESENT)) {
        if (!create)
            return nullptr;
        // A new kernel half PDPT would exist in this address space only
        if (index4(virtAddr) >= kernelHalfSlot)
            kernelPanic("VMM: the kernel half has a fixed set of PDPTs");
        uint64_t frame = newTable();
        if (!frame)
            return nullptr;
        pml4Entry = frame | parentFlags;
    }

    uint64_t& pdptEntry = table(pml4Entry)[index3(virtAddr)];
    if (!(pdptEntry & MMU_PRESENT)) {
        if (!create)
            return nullptr;
        uint64_t frame = newTable();
        if (!frame)
            return nullptr;
        pdptEntry = frame | parentFlags;
    }
    if (pdptEntry & MMU_PDE_TWO_MB)
        return nullptr; // a 1 GB page: not made by atlOS

    return &table(pdptEntry)[index2(virtAddr)];
}

void AddressSpace::invalidate(uint64_t virtAddr) const
{
    // The kernel half is visible in every address space, so also in the active one
    if (isActive() || virtAddr >= physmapBase)
        asm volatile("invlpg (%0)"
                     :
                     : "r"((void*)virtAddr)
                     : "memory");
}

bool AddressSpace::mapPage(uint64_t virtAddr, uint64_t entry)
{
    uint64_t* pd = pdEntry(virtAddr, true);
    if (!pd || (*pd & MMU_PRESENT))
        return false;
    *pd = entry | MMU_PRESENT | MMU_PDE_TWO_MB;
    invalidate(virtAddr);
    return true;
}

void AddressSpace::unmapPage(uint64_t virtAddr, const char* caller)
{
    uint64_t* pd = pdEntry(virtAddr, false);
    if (!pd || !(*pd & MMU_PRESENT)) {
        logMsg(infLog, "VMM: %s: 0x%llx is not mapped\n", caller, virtAddr);
        kernelPanic("VMM: unmapping a page that is not mapped");
    }

    uint64_t entry = *pd;
    *pd = 0;
    invalidate(virtAddr);
    if (entry & entryOwned)
        m_pmm.freeBlocks((void*)(entry & page2MbAddrMask), 1);

    // An empty PD goes back to the PMM (not the ones in the kernel image)
    uint64_t* pdTable = (uint64_t*)((uintptr_t)pd & ~(uintptr_t)(frameSize4Kb - 1));
    for (uint64_t i = 0; i < 512; i++) {
        if (pdTable[i] & MMU_PRESENT)
            return;
    }
    uint64_t& pdptEntry = table(table(m_pml4)[index4(virtAddr)])[index3(virtAddr)];
    uint64_t pdPhys = pdptEntry & tableAddrMask;
    if (pdPhys < (uintptr_t)&kernel_physical_end)
        return;
    pdptEntry = 0;
    freeTableIfOwned(pdPhys);
    logMsg(dbgLog, "VMM: freed an empty PD at 0x%llx\n", pdPhys);
}

// ------------------------------------------------------------
// Kernel pages
// ------------------------------------------------------------

/// First fit over the 2 MB pages from the end of the kernel image to the top
uint64_t AddressSpace::findFreeKernelRange(uint64_t count)
{
    uint64_t start = ((uint64_t)&kernel_virtual_end + pageSize2Mb - 1) & ~(uint64_t)(pageSize2Mb - 1);
    uint64_t run = 0;
    for (uint64_t page = start; page >= start; page += pageSize2Mb) {
        uint64_t* pd = pdEntry(page, false);
        if (pd && (*pd & MMU_PRESENT)) {
            run = 0;
            continue;
        }
        if (++run == count)
            return page - (count - 1) * pageSize2Mb;
    }
    kernelPanic("VMM: no free kernel virtual range");
    return 0;
}

void* AddressSpace::allocatePages(uint64_t count)
{
    uint64_t base = findFreeKernelRange(count);
    for (uint64_t i = 0; i < count; i++) {
        uint64_t phys = (uintptr_t)m_pmm.allocateBlocks(1);
        if (!phys)
            kernelPanic("VMM: out of physical memory");
        logMsg(dbgLog, "VMM: Mapping page: VA=0x%llx\n", base + i * pageSize2Mb);
        if (!mapPage(base + i * pageSize2Mb, phys | MMU_WRITABLE | entryOwned))
            kernelPanic("VMM: out of physical memory for page directory");
    }
    return (void*)base;
}

void AddressSpace::freePages(const void* base, uint64_t count)
{
    for (uint64_t i = 0; i < count; i++)
        unmapPage((uint64_t)base + i * pageSize2Mb, "freePages");
}

void AddressSpace::mapPhysical(const void* virtAddr, const void* physAddr)
{
    uint64_t vAddr = (uintptr_t)virtAddr;
    uint64_t phys = (uintptr_t)physAddr;
    if ((vAddr | phys) & (pageSize2Mb - 1))
        kernelPanic("VMM: mapPhysical: addresses must be 2 MB aligned");
    if (!mapPage(vAddr, phys | MMU_WRITABLE))
        kernelPanic("VMM: mapPhysical: page already mapped!");
}

void AddressSpace::unmapPhysical(const void* virtAddr)
{
    unmapPage((uintptr_t)virtAddr & ~(uintptr_t)(pageSize2Mb - 1), "unmapPhysical");
}

// ------------------------------------------------------------
// User pages
// ------------------------------------------------------------

bool AddressSpace::allocateAt(uint64_t virtAddr, uint64_t count, uint64_t flags)
{
    if (virtAddr & (pageSize2Mb - 1) || virtAddr < userBase || virtAddr >= userEnd
        || count > (userEnd - virtAddr) / pageSize2Mb)
        return false;

    for (uint64_t i = 0; i < count; i++) {
        uint64_t phys = (uintptr_t)m_pmm.allocateBlocks(1);
        if (!phys)
            return false;
        // A user program must never see what the page held before
        memset(physToVirt(phys), 0, pageSize2Mb);
        if (!mapPage(virtAddr + i * pageSize2Mb, phys | (flags & (Writable | User)) | entryOwned)) {
            m_pmm.freeBlocks((void*)phys, 1);
            return false;
        }
    }
    return true;
}

void AddressSpace::freeAt(uint64_t virtAddr, uint64_t count)
{
    for (uint64_t i = 0; i < count; i++)
        unmapPage(virtAddr + i * pageSize2Mb, "freeAt");
}

// ------------------------------------------------------------
// Looking at the tables
// ------------------------------------------------------------

bool AddressSpace::translate(const void* virtAddr, uint64_t& physAddr) const
{
    uint64_t vAddr = (uintptr_t)virtAddr;
    if (canonical(vAddr & 0x0000FFFFFFFFFFFFULL) != vAddr)
        return false;

    uint64_t pml4Entry = table(m_pml4)[index4(vAddr)];
    if (!(pml4Entry & MMU_PRESENT))
        return false;

    uint64_t pdptEntry = table(pml4Entry)[index3(vAddr)];
    if (!(pdptEntry & MMU_PRESENT))
        return false;
    if (pdptEntry & MMU_PDE_TWO_MB) { // 1 GB page
        physAddr = (pdptEntry & 0x000FFFFFC0000000ULL) | (vAddr & (gigabyte - 1));
        return true;
    }

    uint64_t pdEntry = table(pdptEntry)[index2(vAddr)];
    if (!(pdEntry & MMU_PRESENT))
        return false;
    if (pdEntry & MMU_PDE_TWO_MB) {
        physAddr = (pdEntry & page2MbAddrMask) | (vAddr & (pageSize2Mb - 1));
        return true;
    }

    // A 4 KB page table: atlOS makes none, but a walk should not get it wrong
    uint64_t ptEntry = table(pdEntry)[(vAddr >> 12) & 0x1FF];
    if (!(ptEntry & MMU_PRESENT))
        return false;
    physAddr = (ptEntry & tableAddrMask) | (vAddr & 0xFFF);
    return true;
}

void* AddressSpace::virtToPhys(const void* virtAddr) const
{
    uint64_t phys;
    if (!translate(virtAddr, phys))
        return nullptr;
    return (void*)phys;
}

void AddressSpace::forEachMapping(MappingCallback callback, void* arg) const
{
    constexpr uint64_t flagMask = 0xFFF | (1ULL << 63); // low flags and NX

    const uint64_t* pml4Table = table(m_pml4);
    for (uint64_t i = 0; i < 512; i++) {
        if (!(pml4Table[i] & MMU_PRESENT))
            continue;

        const uint64_t* pdpt = table(pml4Table[i]);
        for (uint64_t j = 0; j < 512; j++) {
            uint64_t pdptEntry = pdpt[j];
            if (!(pdptEntry & MMU_PRESENT))
                continue;
            uint64_t base1Gb = canonical((i << 39) | (j << 30));
            if (pdptEntry & MMU_PDE_TWO_MB) {
                callback(base1Gb, pdptEntry & 0x000FFFFFC0000000ULL, gigabyte, pdptEntry & flagMask, arg);
                continue;
            }

            const uint64_t* pd = table(pdptEntry);
            for (uint64_t k = 0; k < 512; k++) {
                uint64_t pdEntry = pd[k];
                if (!(pdEntry & MMU_PRESENT))
                    continue;
                uint64_t base2Mb = base1Gb | (k << 21);
                if (pdEntry & MMU_PDE_TWO_MB) {
                    callback(base2Mb, pdEntry & page2MbAddrMask, pageSize2Mb, pdEntry & flagMask, arg);
                    continue;
                }

                const uint64_t* pt = table(pdEntry);
                for (uint64_t l = 0; l < 512; l++) {
                    if (pt[l] & MMU_PRESENT)
                        callback(base2Mb | (l << 12), pt[l] & tableAddrMask, 4096, pt[l] & flagMask, arg);
                }
            }
        }
    }
}

bool AddressSpace::isUserAccessible(uint64_t virtAddr, uint64_t size, bool write) const
{
    if (virtAddr < userBase || virtAddr >= userEnd || size > userEnd - virtAddr)
        return false;

    // The CPU allows an access only if every level allows it
    uint64_t need = MMU_PRESENT | MMU_USER_MEMORY | (write ? MMU_WRITABLE : 0);
    for (uint64_t page = virtAddr & ~(uint64_t)(pageSize2Mb - 1); page < virtAddr + size; page += pageSize2Mb) {
        uint64_t entry = table(m_pml4)[index4(page)];
        if ((entry & need) != need)
            return false;
        entry = table(entry)[index3(page)];
        if ((entry & need) != need || (entry & MMU_PDE_TWO_MB))
            return false;
        entry = table(entry)[index2(page)];
        if ((entry & need) != need || !(entry & MMU_PDE_TWO_MB))
            return false;
    }
    return true;
}

void AddressSpace::activate() const
{
    writeCr3(m_pml4);
}

bool AddressSpace::isActive() const
{
    return (readCr3() & tableAddrMask) == m_pml4;
}

} // namespace Memory
