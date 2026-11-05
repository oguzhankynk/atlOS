#include <heap/fixed_region_pages.hpp>

namespace Memory {

FixedRegionPages::FixedRegionPages(void* base, size_t size, size_t pageSize)
    : m_base(static_cast<uint8_t*>(base))
    , m_pageSize(pageSize)
    , m_count(pageSize ? size / pageSize : 0)
{
    if (m_count > maxPages)
        m_count = maxPages;
}

// count pages in a row starting at page first, all of them free?
static bool runIsFree(uint64_t used, size_t first, size_t count)
{
    for (size_t page = first; page < first + count; page++) {
        if (used & (1ULL << page))
            return false;
    }
    return true;
}

void* FixedRegionPages::allocatePages(size_t count)
{
    if (count == 0 || count > m_count)
        return nullptr;

    for (size_t first = 0; first + count <= m_count; first++) {
        if (!runIsFree(m_used, first, count))
            continue;
        for (size_t page = first; page < first + count; page++)
            m_used |= 1ULL << page;
        return m_base + first * m_pageSize;
    }
    return nullptr;
}

void FixedRegionPages::freePages(void* pages, size_t count)
{
    auto* start = static_cast<uint8_t*>(pages);
    if (start < m_base || (size_t)(start - m_base) % m_pageSize != 0)
        return;

    size_t first = (size_t)(start - m_base) / m_pageSize;
    for (size_t page = first; page < first + count && page < m_count; page++)
        m_used &= ~(1ULL << page);
}

} // namespace Memory
