// kmalloc and friends on the kernel's Memory::Heap (common/heap/).

#include <core/panic.hpp>
#include <mm/constants.hpp>
#include <mm/heap.hpp>
#include <mm/vmm.hpp>

namespace Memory {

size_t AddressSpacePages::pageSize() const
{
    return pageSize2Mb;
}

void* AddressSpacePages::allocatePages(size_t count)
{
    // Panics when out of memory, so the heap never sees nullptr from here
    return m_space.allocatePages(count);
}

void AddressSpacePages::freePages(void* pages, size_t count)
{
    m_space.freePages(pages, count);
}

// Bound by kernelMain, see setKernelHeap
static Heap* s_kernelHeap {};
static AddressSpace* s_kernelSpace {};

void setKernelHeap(Heap& heap, AddressSpace& space)
{
    s_kernelHeap = &heap;
    s_kernelSpace = &space;
}

Heap& kernelHeap()
{
    if (!s_kernelHeap)
        kernelPanic("kmalloc before Memory::setKernelHeap");
    return *s_kernelHeap;
}

AddressSpace& kernelAddressSpace()
{
    if (!s_kernelSpace)
        kernelPanic("kernelAddressSpace before Memory::setKernelHeap");
    return *s_kernelSpace;
}

} // namespace Memory

void* kmalloc(size_t size)
{
    return Memory::kernelHeap().allocate(size);
}

void* kcalloc(size_t count, size_t size)
{
    return Memory::kernelHeap().allocateZeroed(count, size);
}

void* krealloc(void* block, size_t size)
{
    return Memory::kernelHeap().reallocate(block, size);
}

// A pointer the heap did not hand out (or a double free) is a kernel bug:
// stop where it happened instead of breaking the heap
void kfree(void* block)
{
    if (!Memory::kernelHeap().deallocate(block))
        kernelPanic("kfree: not a block of the kernel heap (or freed twice)");
}
