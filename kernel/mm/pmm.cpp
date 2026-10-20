/**
 * @file pmm.cpp
 * @author Oguzhan KAYNAK ()
 * @brief Physical memory manager: 2 MB blocks and 4 KB frames for page tables
 *
 * Copyright © 2024 Oguzhan KAYNAK
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the “Software”), to deal in the Software
 * without  restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit
 * persons to whom the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
 * PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include <mm/pmm.hpp>

#include <string.h>

#include <core/logger.hpp>

namespace Memory {

/// Enable / Disable logs
static constexpr LogFlag dbgLog = LogFlag::NoLog; // every allocation: very chatty
static constexpr LogFlag infLog = LogFlag::Log;
static constexpr LogFlag errLog = LogFlag::Log;

PhysicalMemoryManager::PhysicalMemoryManager(uint8_t* bitmap, uint64_t bitmapBytes)
    : m_bitmap(bitmap)
    , m_capacity(bitmapBytes * 8)
{
    // Everything is in use until a region is added
    memset(m_bitmap, 0xFF, bitmapBytes);

    logMsg(infLog, "PMM: blockSize:%u capacity:%u blocks \n", blockSize, m_capacity);
}

bool PhysicalMemoryManager::testBlock(uint64_t block) const
{
    return m_bitmap[block / 8] & (1 << (block % 8));
}

void PhysicalMemoryManager::setBlock(uint64_t block)
{
    m_bitmap[block / 8] |= (1 << (block % 8));
}

void PhysicalMemoryManager::unsetBlock(uint64_t block)
{
    m_bitmap[block / 8] &= ~(1 << (block % 8));
}

int64_t PhysicalMemoryManager::firstFreeBlock(uint64_t count) const
{
    logMsg(dbgLog, "PMM: firstFreeBlock: number of requested blocks:%u free block:%u \n", count, m_freeBlocks);

    if (count == 0)
        return -1; // Can't return no memory, error

    uint64_t consecutiveFree = 0; // Counter for consecutive free blocks
    uint64_t startIndex = 0; // Index of the start of the free block series

    for (uint64_t block = 0; block < m_maxBlocks; block++) {
        if (!testBlock(block)) {
            if (consecutiveFree == 0) {
                logMsg(dbgLog, "PMM: firstFreeBlock: the current bit is 0 : %u \n", block);
                startIndex = block; // Save the start index of the series
            }
            consecutiveFree++;

            // If a series of sufficient length is found
            if (consecutiveFree == count)
                return static_cast<int64_t>(startIndex);
        } else {
            if (consecutiveFree != 0)
                logMsg(dbgLog, "PMM: firstFreeBlock: the current bit is 1 : %u \n", consecutiveFree);

            consecutiveFree = 0; // Reset if a used block is encountered
        }
    }

    return -1; // No free region of memory large enough
}

void PhysicalMemoryManager::addRegion(const void* base, uint64_t size)
{
    if ((uint64_t)base % blockSize != 0) {
        logMsg(errLog, "PMM: addRegion: base 0x%llx not aligned\n", (uint64_t)base);
        return;
    }

    uint64_t block = (uint64_t)base / blockSize;
    uint64_t endBlock = block + size / blockSize;

    if (endBlock > m_capacity) {
        logMsg(errLog, "PMM: memory above %u blocks is ignored\n", m_capacity);
        endBlock = m_capacity;
    }

    // Block 0 would be returned as nullptr, see the class comment
    if (block == 0)
        block = 1;

    logMsg(infLog, "PMM: addRegion: start_indx: %u num_blocks:%u ", block, endBlock > block ? endBlock - block : 0);

    for (; block < endBlock; block++) {
        if (testBlock(block)) {
            unsetBlock(block);
            m_freeBlocks++;
        }
    }

    if (endBlock > m_maxBlocks)
        m_maxBlocks = endBlock;

    logMsg(infLog, " freeBlocks:%u \n", m_freeBlocks);
}

void PhysicalMemoryManager::reserveRegion(const void* base, uint64_t size)
{
    // Cover every block the region touches, even if it is not block aligned
    uint64_t block = (uint64_t)base / blockSize;
    uint64_t endBlock = ((uint64_t)base + size + blockSize - 1) / blockSize;

    // Blocks above m_maxBlocks were never added, so they are already in use
    if (endBlock > m_maxBlocks)
        endBlock = m_maxBlocks;

    logMsg(infLog, "PMM: reserveRegion: start_indx: %u end_indx:%u ", block, endBlock);

    for (; block < endBlock; block++) {
        if (!testBlock(block)) {
            setBlock(block);
            m_freeBlocks--;
        }
    }

    logMsg(infLog, " freeBlocks:%u \n", m_freeBlocks);
}

void* PhysicalMemoryManager::allocateBlocks(uint64_t count)
{
    logMsg(dbgLog, "PMM: allocateBlocks: num_blocks:%u \n", count);

    // If # of free blocks left is not enough, return
    if (count == 0 || m_freeBlocks < count)
        return nullptr;

    int64_t startingBlock = firstFreeBlock(count);
    logMsg(dbgLog, "PMM: firstFreeBlock starting_block : %d \n", startingBlock);

    if (startingBlock < 0)
        return nullptr; // Couldn't find that many blocks in a row to allocate

    for (uint64_t i = 0; i < count; i++)
        setBlock(startingBlock + i);

    m_freeBlocks -= count;

    uint64_t address = startingBlock * blockSize;

    logMsg(dbgLog, "PMM: allocateBlocks: freeBlocks: %u  address:0x%llx \n", m_freeBlocks, address);
    return reinterpret_cast<void*>(address);
}

void PhysicalMemoryManager::freeBlocks(const void* base, uint64_t count)
{
    if ((uint64_t)base % blockSize != 0) {
        logMsg(errLog, "PMM: freeBlocks: address 0x%llx not aligned\n", (uint64_t)base);
        return;
    }

    uint64_t startingBlock = (uint64_t)base / blockSize;

    for (uint64_t i = 0; i < count; i++) {
        uint64_t block = startingBlock + i;

        // Freeing a block that is not allocated would corrupt m_freeBlocks
        if (block >= m_maxBlocks || !testBlock(block)) {
            logMsg(errLog, "PMM: freeBlocks: block %u is not allocated\n", block);
            continue;
        }

        unsetBlock(block);
        m_freeBlocks++;
    }
}

// ------------------------------------------------------------
// 4 KB frames
// ------------------------------------------------------------

void* PhysicalMemoryManager::takeFrame(FramePool& pool)
{
    for (uint64_t word = 0; word < framesPerBlock / 64; word++) {
        if (pool.bitmap[word] == ~0ULL)
            continue;

        uint64_t bit = __builtin_ctzll(~pool.bitmap[word]);
        pool.bitmap[word] |= (1ULL << bit);
        pool.used++;

        return reinterpret_cast<void*>(pool.base + (word * 64 + bit) * frameSize4Kb);
    }

    return nullptr;
}

void* PhysicalMemoryManager::allocateFrame()
{
    for (uint32_t i = 0; i < m_framePoolCount; i++) {
        if (m_framePools[i].used < framesPerBlock)
            return takeFrame(m_framePools[i]);
    }

    if (m_framePoolCount == maxFramePools) {
        logMsg(errLog, "PMM: allocateFrame: all %u frame pools are full\n", maxFramePools);
        return nullptr;
    }

    void* block = allocateBlocks(1);
    if (block == nullptr)
        return nullptr;

    FramePool& pool = m_framePools[m_framePoolCount++];
    memset(&pool, 0, sizeof(pool));
    pool.base = reinterpret_cast<uint64_t>(block);

    logMsg(dbgLog, "PMM: allocateFrame: new frame pool at 0x%llx \n", pool.base);
    return takeFrame(pool);
}

void PhysicalMemoryManager::freeFrame(const void* frame)
{
    uint64_t address = reinterpret_cast<uint64_t>(frame);

    for (uint32_t i = 0; i < m_framePoolCount; i++) {
        FramePool& pool = m_framePools[i];
        if (address < pool.base || address >= pool.base + pageSize2Mb)
            continue;

        uint64_t index = (address - pool.base) / frameSize4Kb;
        uint64_t mask = 1ULL << (index % 64);

        if (address % frameSize4Kb != 0 || !(pool.bitmap[index / 64] & mask)) {
            logMsg(errLog, "PMM: freeFrame: frame 0x%llx is not allocated\n", address);
            return;
        }

        pool.bitmap[index / 64] &= ~mask;
        pool.used--;

        // Give an empty pool back to the block allocator
        if (pool.used == 0) {
            freeBlocks(reinterpret_cast<void*>(pool.base), 1);
            m_framePools[i] = m_framePools[--m_framePoolCount];
        }
        return;
    }

    logMsg(errLog, "PMM: freeFrame: 0x%llx does not belong to any frame pool\n", address);
}

} // namespace Memory
