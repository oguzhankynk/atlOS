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

} // namespace Memory
#endif
