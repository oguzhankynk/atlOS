#pragma once

#include <stdint.h>

#include <mm/constants.hpp>

namespace Memory {

class PhysicalMemoryManager;

/**
 * @brief The kernel's virtual address space.
 *
 * 2 MB pages between kernel_virtual_end and the top of the address space,
 * backed by blocks of the given PhysicalMemoryManager. The page tables are
 * reached through a recursive mapping in PML4[510], which only works for the
 * active tables (CR3): an AddressSpace works on the tables that are loaded.
 * Today there is one, built by boot.S.
 */
class AddressSpace {
public:
    /**
     * @brief Take over the active page tables and install the recursive mapping.
     * @param pmm Source of the physical pages and page directories; must outlive this object.
     */
    explicit AddressSpace(PhysicalMemoryManager& pmm);

    AddressSpace(const AddressSpace&) = delete;
    AddressSpace& operator=(const AddressSpace&) = delete;

    /**
     * @brief Map count new 2 MB pages at a free, contiguous virtual range.
     * Panics when there is no free range or no physical memory.
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
     * @brief Translate a virtual address by walking the page tables.
     * @return The physical address, or nullptr if it is not mapped.
     */
    void* virtToPhys(const void* virtAddr) const;

private:
    uint64_t* getOrCreatePdEntry(uintptr_t vAddr);
    void clearPdEntry(uintptr_t vAddr, uint64_t* pdEntry);
    void mapPages(uint64_t pageIndex, uint64_t count);
    void unmapPages(uint64_t pageIndex, uint64_t count);

    PhysicalMemoryManager& m_pmm;
};

} // namespace Memory
