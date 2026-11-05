/**
 * @file heap.hpp
 * @author Oguzhan KAYNAK
 * @brief A heap: a free list, first fit, with neighbours merged on free.
 *
 * The same code is the kernel's kmalloc and a user program's malloc; only
 * the PageSource behind it differs. Not thread safe: the scheduler is
 * cooperative and interrupt handlers do not allocate. See README.md.
 */

#pragma once

#include <heap/page_source.hpp>
#include <stddef.h>
#include <stdint.h>

namespace Memory {

class Heap {
public:
    /// Every block starts at a multiple of this (the System V ABI's largest alignment)
    static constexpr size_t alignment = 16;

    struct Statistics {
        uint64_t reserved; ///< bytes of the pages taken from the source
        uint64_t inUse; ///< bytes handed out and not freed yet (the blocks' usable sizes)
        uint32_t chunks; ///< runs of pages taken from the source
        uint32_t freeBlocks; ///< entries of the free list
    };

    /// Takes nothing yet: the first allocation asks pages for the first chunk.
    explicit Heap(PageSource& pages);

    /// Gives every chunk back, freed or not.
    ~Heap();

    Heap(const Heap&) = delete;
    Heap& operator=(const Heap&) = delete;

    /// size bytes (0 counts as 1), aligned to 16; nullptr if the source has no more pages.
    void* allocate(size_t size);

    /// count * size bytes, zeroed; nullptr on overflow or when out of memory.
    void* allocateZeroed(size_t count, size_t size);

    /**
     * @brief Resize to size bytes, keeping the contents (up to the smaller
     * size). nullptr: allocate; size 0: free and return nullptr. On failure
     * the old block stays as it was and nullptr is returned.
     */
    void* reallocate(void* block, size_t size);

    /**
     * @brief Give a block back. nullptr is fine.
     * @return false if it is not a block this heap handed out, or it is free
     * already (a double free); nothing changes then.
     */
    bool deallocate(void* block);

    /// Bytes the block can hold (at least what was asked for); 0 if it is not a used block.
    size_t usableSize(const void* block) const;

    Statistics statistics() const;

private:
    struct Chunk;
    struct Block;

    static Block*& nextFree(Block* block);
    Chunk* chunkOf(const Block* block) const;
    Block* blockOf(const void* pointer) const;
    bool grow(size_t blockSize);
    Block* insertFree(Block* block);
    void releaseIfEmpty(Block* block);

    PageSource& m_pages;
    Chunk* m_chunks {}; // every chunk, newest first
    Block* m_free {}; // free blocks, sorted by address
    uint64_t m_reserved {};
    uint64_t m_inUse {};
};

} // namespace Memory
