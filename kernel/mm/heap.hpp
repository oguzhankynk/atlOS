#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void* kmalloc(size_t);
void* krealloc(void*, size_t);
void* kcalloc(size_t, size_t);
void kfree(void*);

#ifdef __cplusplus
}

namespace Memory {

class AddressSpace;

/**
 * @brief Set the address space that the pages behind kmalloc come from.
 *
 * liballoc asks for pages through C hooks that can not be given an object, so
 * kernelMain binds the kernel's address space once, before the first kmalloc.
 */
void setHeapAddressSpace(AddressSpace& space);

/**
 * @brief The kernel's address space, as bound with setHeapAddressSpace. What
 * Threading::Thread(entry) takes its stacks from. Panics if none is bound.
 */
AddressSpace& kernelAddressSpace();

struct HeapStatistics {
    uint64_t reserved; ///< bytes of pages the heap took from the address space
    uint64_t inUse; ///< bytes handed out by kmalloc and not freed yet
};

HeapStatistics heapStatistics();

} // namespace Memory
#endif
