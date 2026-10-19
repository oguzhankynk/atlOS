// Hooks that connect liballoc (third party) to the kernel's address space.

#include <core/panic.hpp>
#include <mm/heap.hpp>
#include <mm/liballoc/liballoc_1_1.h>
#include <mm/vmm.hpp>

// Bound by kernelMain, see Memory::setHeapAddressSpace
static Memory::AddressSpace* s_heapAddressSpace {};

void Memory::setHeapAddressSpace(AddressSpace& space)
{
    s_heapAddressSpace = &space;
}

static Memory::AddressSpace& heapAddressSpace()
{
    if (!s_heapAddressSpace)
        kernelPanic("kmalloc before Memory::setHeapAddressSpace");
    return *s_heapAddressSpace;
}

// The scheduler is cooperative and interrupt handlers do not allocate, so
// liballoc needs no lock.
int liballoc_lock()
{
    return 0;
}

int liballoc_unlock()
{
    return 0;
}

/** Allocate num_pages pages (of the page size set up in liballoc_init).
 * \return NULL if the pages were not allocated (the address space panics instead).
 */
void* liballoc_alloc(size_t num_pages)
{
    return heapAddressSpace().allocatePages(num_pages);
}

/** Free pages returned by liballoc_alloc.
 * \return 0 if the memory was successfully freed.
 */
int liballoc_free(void* addr, size_t num_pages)
{
    heapAddressSpace().freePages(addr, (uint64_t)num_pages);
    return 0;
}
