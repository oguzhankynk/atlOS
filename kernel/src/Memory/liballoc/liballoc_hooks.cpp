#include "Memory/liballoc/liballoc_1_1.h"
#include <Memory/Memory.hpp>
#include <Memory/VirtualMemory.hpp>

int liballoc_lock()
{
    return 0;
}

int liballoc_unlock()
{
    return 0;
}

/** This is the hook into the local system which allocates pages. It
 * accepts an integer parameter which is the number of pages
 * required.  The page size was set up in the liballoc_init function.
 *
 * \return NULL if the pages were not allocated.
 * \return A pointer to the allocated memory.
 */
void *liballoc_alloc(size_t num_pages)
{
    return Memory::VirtualMemory::allocatePage(num_pages);
}

/** This frees previously allocated memory. The void* parameter passed
 * to the function is the exact same value returned from a previous
 * liballoc_alloc call.
 *
 * The integer value is the number of pages to free.
 *
 * \return 0 if the memory was successfully freed.
 */
extern int liballoc_free(void *addr, size_t num_pages)
{
    return Memory::VirtualMemory::freePage(addr, (uint64_t)num_pages);
}