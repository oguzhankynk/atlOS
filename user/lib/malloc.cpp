// malloc and free: the kernel's heap code (common/heap/) on the region the
// kernel mapped for this program.

#include "atlos.h"
#include "heap_glue.hpp"

#include <heap/fixed_region_pages.hpp>
#include <heap/heap.hpp>

// There is no operator new for user programs: the two objects are built in
// place. Their constructors must not run before main as global objects
// would (crt0 runs no constructors)
inline void* operator new(unsigned long, void* where) noexcept
{
    return where;
}

alignas(Memory::FixedRegionPages) static unsigned char s_pagesStorage[sizeof(Memory::FixedRegionPages)];
alignas(Memory::Heap) static unsigned char s_heapStorage[sizeof(Memory::Heap)];
static Memory::Heap* s_heap;
static unsigned long s_heapBase;
static unsigned long s_heapSize;

void atlos_heap_init(unsigned long base, unsigned long size, unsigned long pageSize)
{
    s_heapBase = base;
    s_heapSize = size;
    if (!base || !size || !pageSize)
        return; // no heap: malloc returns NULL
    auto* pages = new (s_pagesStorage) Memory::FixedRegionPages(reinterpret_cast<void*>(base), size, pageSize);
    s_heap = new (s_heapStorage) Memory::Heap(*pages);
}

extern "C" void* malloc(unsigned long size)
{
    return s_heap ? s_heap->allocate(size) : nullptr;
}

extern "C" void* calloc(unsigned long count, unsigned long size)
{
    return s_heap ? s_heap->allocateZeroed(count, size) : nullptr;
}

extern "C" void* realloc(void* block, unsigned long size)
{
    return s_heap ? s_heap->reallocate(block, size) : nullptr;
}

// A pointer malloc did not hand out, or a second free: the program ends, as
// it would on Linux (abort, 128 + SIGABRT)
extern "C" void free(void* block)
{
    if (!block)
        return;
    if (!s_heap || !s_heap->deallocate(block)) {
        print("free: not a block from malloc (or freed twice)\n");
        exit(134);
    }
}

extern "C" void heap_info(unsigned long* base, unsigned long* size, unsigned long* inUse)
{
    *base = s_heapBase;
    *size = s_heapSize;
    *inUse = s_heap ? s_heap->statistics().inUse : 0;
}
