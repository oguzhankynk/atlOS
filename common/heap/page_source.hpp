/**
 * @file page_source.hpp
 * @author Oguzhan KAYNAK
 * @brief Where a Heap gets its memory from: whole, contiguous pages.
 *
 * The kernel's heap takes them from its address space, a user program's from
 * the fixed region the kernel mapped for it (FixedRegionPages). See README.md.
 */

#pragma once

#include <stddef.h>

namespace Memory {

class PageSource {
public:
    /// Bytes per page; the heap asks for whole pages.
    virtual size_t pageSize() const = 0;

    /// count contiguous pages, writable; nullptr if there are none.
    virtual void* allocatePages(size_t count) = 0;

    /// Give back pages that allocatePages handed out, with the same count.
    virtual void freePages(void* pages, size_t count) = 0;

protected:
    // Never deleted through the interface
    ~PageSource() = default;
};

} // namespace Memory
