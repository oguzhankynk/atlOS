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

#include <heap/heap.hpp>

namespace Memory {

class AddressSpace;

/// The kernel heap's pages: 2 MB pages of an address space, anywhere above the kernel image.
class AddressSpacePages final : public PageSource {
public:
    explicit AddressSpacePages(AddressSpace& space)
        : m_space(space)
    {
    }

    size_t pageSize() const override;
    void* allocatePages(size_t count) override;
    void freePages(void* pages, size_t count) override;

private:
    AddressSpace& m_space;
};

/**
 * @brief Bind the heap behind kmalloc, new and delete, and the kernel's
 * address space. They are reached from code that can not be handed an object
 * (operator new, C callers), so kernelMain binds them once, before the first
 * kmalloc. A boundary global, see docs/style.md.
 */
void setKernelHeap(Heap& heap, AddressSpace& space);

/// The heap bound with setKernelHeap; panics if none is.
Heap& kernelHeap();

/**
 * @brief The kernel's address space, as bound with setKernelHeap. What
 * Threading::Thread(entry) takes its stacks from. Panics if none is bound.
 */
AddressSpace& kernelAddressSpace();

} // namespace Memory
#endif
