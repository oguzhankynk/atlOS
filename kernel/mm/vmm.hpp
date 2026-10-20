#pragma once

#include <stdint.h>

#include <mm/constants.hpp>

namespace Memory {

class PhysicalMemoryManager;

/// All physical memory, mapped with 2 MB pages at physmapBase + physical address (PML4[256])
static constexpr uint64_t physmapBase = 0xFFFF800000000000ULL;

/// How much physical memory the physmap covers (and so the PMM may hand out)
static constexpr uint64_t physmapSize = 64ULL * 1024 * 1024 * 1024;

/// The part of the lower half user programs live in: PML4[1..255]. PML4[0]
/// holds the identity mapping of the first megabytes, for the kernel only.
static constexpr uint64_t userBase = 0x0000008000000000ULL;
static constexpr uint64_t userEnd = 0x0000800000000000ULL;

/// The kernel's view of a physical address (through the physmap)
inline void* physToVirt(uint64_t physAddr)
{
    return reinterpret_cast<void*>(physmapBase + physAddr);
}

/**
 * @brief A virtual address space: one PML4 and the tables below it.
 *
 * Page tables are reached through the physmap (physmapBase + their physical
 * address), so an AddressSpace works the same whether its tables are loaded
 * in CR3 or not. Every address space shares the kernel's PML4[0] (identity
 * mapping, kernel only) and PML4[256..511] (physmap, kernel image, kernel
 * pages): a change there is seen by all of them. Only 2 MB pages are used.
 */
class AddressSpace {
public:
    enum PageFlags : uint64_t {
        Writable = 1 << 1,
        User = 1 << 2, ///< reachable from ring 3
    };

    /**
     * @brief The kernel's address space: takes over the tables boot.S built
     * (the active CR3) and adds the physmap. Construct it once, first.
     * @param pmm Source of the physical pages and page directories; must outlive this object.
     */
    explicit AddressSpace(PhysicalMemoryManager& pmm);

    /**
     * @brief A new, empty address space for a user program: the kernel's
     * shared entries and nothing in the user range yet. Not active.
     */
    AddressSpace(PhysicalMemoryManager& pmm, const AddressSpace& kernel);

    /// A user address space gives back its pages and tables. Must not be active.
    ~AddressSpace();

    AddressSpace(const AddressSpace&) = delete;
    AddressSpace& operator=(const AddressSpace&) = delete;

    /**
     * @brief Map count new 2 MB pages at a free, contiguous virtual range
     * above the kernel image (kernel address space). Panics when there is no
     * free range or no physical memory.
     */
    void* allocatePages(uint64_t count);

    /// Unmap pages from allocatePages and give their physical blocks back
    void freePages(const void* base, uint64_t count);

    /**
     * @brief Map a 2 MB page to a given physical address (e.g. for MMIO).
     * The physical memory is not taken from or returned to the PMM.
     */
    void mapPhysical(const void* virtAddr, const void* physAddr);

    /// Remove a mapping made by mapPhysical. The physical memory is not freed.
    void unmapPhysical(const void* virtAddr);

    /**
     * @brief Map count new, zeroed 2 MB pages at virtAddr in the user range
     * (user address spaces). flags: Writable, User.
     * @return false if a page is mapped already, the range is not in
     * [userBase, userEnd), or memory ran out (pages mapped so far stay).
     */
    bool allocateAt(uint64_t virtAddr, uint64_t count, uint64_t flags);

    /// Unmap pages from allocateAt and give their blocks back.
    void freeAt(uint64_t virtAddr, uint64_t count);

    /**
     * @brief Translate a virtual address by walking the page tables.
     * @return The physical address, or nullptr if it is not mapped.
     */
    void* virtToPhys(const void* virtAddr) const;

    /**
     * @brief Like virtToPhys, but tells "not mapped" apart from "mapped at
     * physical 0" (the identity mapping starts there).
     */
    bool translate(const void* virtAddr, uint64_t& physAddr) const;

    /// Called once per mapped page: its size is 4 KB, 2 MB or 1 GB; flags are the entry's low bits.
    using MappingCallback = void (*)(uint64_t virtAddr, uint64_t physAddr, uint64_t size, uint64_t flags, void* arg);

    /// Walk the page tables and report every mapped page, in ascending virtual order.
    void forEachMapping(MappingCallback callback, void* arg) const;

    /// Load this address space into CR3.
    void activate() const;

    bool isActive() const;

    /// Physical address of the PML4 (what goes into CR3).
    uint64_t pml4Physical() const { return m_pml4; }

private:
    uint64_t* pdEntry(uint64_t virtAddr, bool create);
    uint64_t newTable();
    void freeTableIfOwned(uint64_t tablePhys);
    void invalidate(uint64_t virtAddr) const;
    bool mapPage(uint64_t virtAddr, uint64_t entry);
    void unmapPage(uint64_t virtAddr, const char* caller);
    uint64_t findFreeKernelRange(uint64_t count);

    PhysicalMemoryManager& m_pmm;
    uint64_t m_pml4;
    bool m_isKernel;
};

} // namespace Memory
