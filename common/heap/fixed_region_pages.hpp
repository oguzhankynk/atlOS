/**
 * @file fixed_region_pages.hpp
 * @author Oguzhan KAYNAK
 * @brief A PageSource over one region given at construction: a user
 * program's heap, which the kernel maps before the program starts. A bitmap
 * of the pages, first fit, like the kernel's physical memory manager.
 */

#pragma once

#include <heap/page_source.hpp>
#include <stdint.h>

namespace Memory {

class FixedRegionPages final : public PageSource {
public:
    /// One bit per page in a uint64_t
    static constexpr size_t maxPages = 64;

    /// The whole pages of [base, base + size); more than maxPages are left unused.
    FixedRegionPages(void* base, size_t size, size_t pageSize);

    size_t pageSize() const override { return m_pageSize; }
    void* allocatePages(size_t count) override;
    void freePages(void* pages, size_t count) override;

    /// Pages in the region.
    size_t pageCount() const { return m_count; }

private:
    uint8_t* m_base;
    size_t m_pageSize;
    size_t m_count;
    uint64_t m_used {}; // bit i: page i is handed out
};

} // namespace Memory
