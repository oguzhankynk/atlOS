#include <heap/heap.hpp>

namespace Memory {

// The start of a run of pages from the source: the link to the next chunk,
// then blocks up to the end of the pages
struct Heap::Chunk {
    Chunk* next;
    size_t pages;
};

// The header in front of every block. A free block keeps its link in the
// free list in the first bytes behind the header, which nobody uses while
// the block is free
struct Heap::Block {
    uint64_t size; // bytes, header included; a multiple of alignment
    uint64_t magic; // usedMagic or freeMagic: what free checks a pointer with
};

static constexpr uint64_t usedMagic = 0xA110CA7EDB10C000ULL;
static constexpr uint64_t freeMagic = 0xF4EEB10CF4EEB10CULL;

// The largest size_t (the kernel's libc has no SIZE_MAX)
static constexpr size_t maxSize = ~(size_t)0;

static constexpr size_t headerSize = 16;
static constexpr size_t chunkHeaderSize = 16;

// The smallest block: its header and room for the free list link
static constexpr size_t minBlockSize = headerSize + Heap::alignment;

static constexpr size_t roundUp(size_t value, size_t multiple)
{
    return (value + multiple - 1) / multiple * multiple;
}

// Byte loops: the heap is built for user programs too, which have no libc
static void copyBytes(uint8_t* to, const uint8_t* from, size_t count)
{
    for (size_t i = 0; i < count; i++)
        to[i] = from[i];
}

static void zeroBytes(uint8_t* to, size_t count)
{
    for (size_t i = 0; i < count; i++)
        to[i] = 0;
}

// A free block's link in the free list: right behind its header
Heap::Block*& Heap::nextFree(Block* block)
{
    return *reinterpret_cast<Block**>(block + 1);
}

// The block size (header included) for size bytes; 0 if it does not fit a size_t
static size_t blockSizeFor(size_t size)
{
    if (size == 0)
        size = 1;
    if (size > maxSize - headerSize - Heap::alignment)
        return 0;
    size_t blockSize = roundUp(size, Heap::alignment) + headerSize;
    return blockSize < minBlockSize ? minBlockSize : blockSize;
}

Heap::Heap(PageSource& pages)
    : m_pages(pages)
{
    static_assert(sizeof(Block) == headerSize && sizeof(Chunk) == chunkHeaderSize, "headers keep the alignment");
}

Heap::~Heap()
{
    while (m_chunks) {
        Chunk* chunk = m_chunks;
        m_chunks = chunk->next;
        m_pages.freePages(chunk, chunk->pages);
    }
}

void* Heap::allocate(size_t size)
{
    size_t need = blockSizeFor(size);
    if (need == 0)
        return nullptr;

    // First fit; if no free block is large enough, one more chunk and again
    for (int attempt = 0; attempt < 2; attempt++) {
        Block** link = &m_free;
        for (Block* block = m_free; block; link = &nextFree(block), block = nextFree(block)) {
            if (block->size < need)
                continue;

            if (block->size - need >= minBlockSize) {
                // Split: the front is handed out, the rest stays free in its place
                auto* rest = reinterpret_cast<Block*>(reinterpret_cast<uint8_t*>(block) + need);
                rest->size = block->size - need;
                rest->magic = freeMagic;
                nextFree(rest) = nextFree(block);
                *link = rest;
                block->size = need;
            } else {
                // Too little left for a block of its own: the whole block goes
                *link = nextFree(block);
            }

            block->magic = usedMagic;
            m_inUse += block->size - headerSize;
            return block + 1;
        }

        if (attempt == 0 && !grow(need))
            return nullptr;
    }
    return nullptr;
}

void* Heap::allocateZeroed(size_t count, size_t size)
{
    if (size != 0 && count > maxSize / size)
        return nullptr;

    // The pages from the source are not zeroed, and freed blocks keep their bytes
    void* block = allocate(count * size);
    if (block)
        zeroBytes(static_cast<uint8_t*>(block), count * size);
    return block;
}

void* Heap::reallocate(void* block, size_t size)
{
    if (!block)
        return allocate(size);
    if (size == 0) {
        deallocate(block);
        return nullptr;
    }

    size_t usable = usableSize(block);
    if (usable == 0)
        return nullptr;
    if (usable >= size)
        return block; // it fits already (a smaller size keeps the block as it is)

    void* moved = allocate(size);
    if (!moved)
        return nullptr;
    copyBytes(static_cast<uint8_t*>(moved), static_cast<const uint8_t*>(block), usable);
    deallocate(block);
    return moved;
}

bool Heap::deallocate(void* pointer)
{
    if (!pointer)
        return true;

    Block* block = blockOf(pointer);
    if (!block)
        return false;

    m_inUse -= block->size - headerSize;
    block->magic = freeMagic;
    releaseIfEmpty(insertFree(block));
    return true;
}

size_t Heap::usableSize(const void* pointer) const
{
    const Block* block = blockOf(pointer);
    return block ? block->size - headerSize : 0;
}

Heap::Statistics Heap::statistics() const
{
    Statistics statistics { m_reserved, m_inUse, 0, 0 };
    for (Chunk* chunk = m_chunks; chunk; chunk = chunk->next)
        statistics.chunks++;
    for (Block* block = m_free; block; block = nextFree(block))
        statistics.freeBlocks++;
    return statistics;
}

// The chunk whose blocks include block's address, nullptr if none does
Heap::Chunk* Heap::chunkOf(const Block* block) const
{
    auto address = reinterpret_cast<uintptr_t>(block);
    for (Chunk* chunk = m_chunks; chunk; chunk = chunk->next) {
        auto start = reinterpret_cast<uintptr_t>(chunk) + chunkHeaderSize;
        uintptr_t end = reinterpret_cast<uintptr_t>(chunk) + chunk->pages * m_pages.pageSize();
        if (address >= start && address < end)
            return chunk;
    }
    return nullptr;
}

// The used block a pointer from allocate belongs to, nullptr if it is none:
// not aligned, outside every chunk, a free block, or a header that is broken
Heap::Block* Heap::blockOf(const void* pointer) const
{
    auto address = reinterpret_cast<uintptr_t>(pointer);
    if (address % alignment != 0 || address < headerSize)
        return nullptr;

    auto* block = reinterpret_cast<Block*>(address - headerSize);
    Chunk* chunk = chunkOf(block);
    if (!chunk || block->magic != usedMagic)
        return nullptr;

    uintptr_t end = reinterpret_cast<uintptr_t>(chunk) + chunk->pages * m_pages.pageSize();
    if (block->size < minBlockSize || block->size % alignment != 0 || block->size > end - (address - headerSize))
        return nullptr;
    return block;
}

// One more chunk, large enough for a block of blockSize bytes
bool Heap::grow(size_t blockSize)
{
    size_t pageSize = m_pages.pageSize();
    if (blockSize > maxSize - chunkHeaderSize - pageSize)
        return false;
    size_t pages = roundUp(blockSize + chunkHeaderSize, pageSize) / pageSize;

    auto* chunk = static_cast<Chunk*>(m_pages.allocatePages(pages));
    if (!chunk)
        return false;
    chunk->pages = pages;
    chunk->next = m_chunks;
    m_chunks = chunk;
    m_reserved += pages * pageSize;

    // The whole chunk is one free block. It is never merged with a block of
    // another chunk: a chunk header always lies between them
    auto* block = reinterpret_cast<Block*>(reinterpret_cast<uint8_t*>(chunk) + chunkHeaderSize);
    block->size = pages * pageSize - chunkHeaderSize;
    block->magic = freeMagic;
    insertFree(block);
    return true;
}

// Put a free block into the list at its address and merge it with the free
// neighbours it touches. Returns the block it ended up in
Heap::Block* Heap::insertFree(Block* block)
{
    Block* previous = nullptr;
    Block* next = m_free;
    while (next && next < block) {
        previous = next;
        next = nextFree(next);
    }

    nextFree(block) = next;
    if (previous)
        nextFree(previous) = block;
    else
        m_free = block;

    // The block right behind it is free: one block
    if (next && reinterpret_cast<uint8_t*>(block) + block->size == reinterpret_cast<uint8_t*>(next)) {
        block->size += next->size;
        nextFree(block) = nextFree(next);
        next->magic = 0;
    }

    // The block right in front of it is free: one block
    if (previous && reinterpret_cast<uint8_t*>(previous) + previous->size == reinterpret_cast<uint8_t*>(block)) {
        previous->size += block->size;
        nextFree(previous) = nextFree(block);
        block->magic = 0;
        block = previous;
    }
    return block;
}

// A free block that covers its whole chunk: the pages go back to the source
void Heap::releaseIfEmpty(Block* block)
{
    Chunk* chunk = chunkOf(block);
    size_t chunkBytes = chunk->pages * m_pages.pageSize();
    if (reinterpret_cast<uint8_t*>(block) != reinterpret_cast<uint8_t*>(chunk) + chunkHeaderSize
        || block->size != chunkBytes - chunkHeaderSize)
        return;

    for (Block** link = &m_free; *link; link = &nextFree(*link)) {
        if (*link == block) {
            *link = nextFree(block);
            break;
        }
    }
    for (Chunk** link = &m_chunks; *link; link = &(*link)->next) {
        if (*link == chunk) {
            *link = chunk->next;
            break;
        }
    }

    m_reserved -= chunkBytes;
    m_pages.freePages(chunk, chunk->pages);
}

} // namespace Memory
