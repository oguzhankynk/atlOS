/**
 * @file pmm.cpp
 * @author Oguzhan KAYNAK ()
 * @brief
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
#include <mm/constants.hpp>
#include <mm/memory.hpp>

namespace Memory {
namespace PhysicalMemory {
    /// Enable / Disable logs
    static constexpr LogFlag dbgLog = LogFlag::Log;
    static constexpr LogFlag infLog = LogFlag::Log;
    static constexpr LogFlag errLog = LogFlag::Log;

    /**
     * @brief Maximum physical memoryy size (byte)
     *
     */
    static constexpr uint64_t maxMemorySize = 256ULL * 1024ULL * 1024ULL * 1024ULL;

    /**
     * @brief Maximum count of physical pages
     *
     */
    static constexpr uint64_t maxBlocks = maxMemorySize / pageSize2Mb;

    /**
     * @brief Physical memory bitmap size (byte)
     *
     */
    static constexpr uint64_t bitmapSize = (maxBlocks + 7) / 8;

    /**
     * @brief Bitmap for manege physical memory
     *
     */
    static uint8_t s_bitmap[bitmapSize];

    static uint64_t s_blockSize {};

    static uint64_t s_maxBlocks {};

    static uint64_t s_freeBlocks {};

    static inline void setBlock(uint64_t bit)
    {
        s_bitmap[bit / 8] |= (1 << (bit % 8));
    }

    static inline void unsetBlock(uint64_t bit)
    {
        s_bitmap[bit / 8] &= ~(1 << (bit % 8));
    }

    static inline bool testBlock(uint64_t bit)
    {
        return s_bitmap[bit / 8] & (1 << (bit % 8));
    }

    static int64_t firstFreeBlock(uint64_t numBlocks)
    {
        logMsg(infLog, "PMM: firstFreeBlock: number of requested blocks:%u free block:%u \n", numBlocks, s_freeBlocks);

        if (numBlocks == 0)
            return -1; // Can't return no memory, error

        uint64_t consecutiveFree = 0; // Counter for consecutive 0 bits
        uint64_t startIndex = 0; // Index of the start of the free block series

        for (uint64_t bitIndex = 0; bitIndex < s_maxBlocks; bitIndex++) {
            uint64_t byteIndex = bitIndex / 8;
            uint8_t bitOffset = bitIndex % 8;

            // Check if the current bit is 0
            if (!(s_bitmap[byteIndex] & (1 << bitOffset))) {
                if (consecutiveFree == 0) {
                    logMsg(dbgLog, "PMM: firstFreeBlock: the current bit is 0 : %u \n", bitIndex);
                    startIndex = bitIndex; // Save the start index of the series
                }
                consecutiveFree++;

                // If a series of sufficient length is found
                if (consecutiveFree == numBlocks) {
                    return static_cast<int64_t>(startIndex);
                }
            } else {
                if (consecutiveFree != 0)
                    logMsg(dbgLog, "PMM: firstFreeBlock: the current bit is 1 : %u \n", consecutiveFree);

                consecutiveFree = 0; // Reset if a 1 bit is encountered
            }
        }

        return -1; // No free region of memory large enough
    }

    void init()
    {
        // set all memory in use
        memset(s_bitmap, 0xFF, sizeof(s_bitmap));

        s_blockSize = pageSize2Mb;
        s_maxBlocks = 0;
        s_freeBlocks = 0;

        logMsg(infLog, "PMM: init: blockSize:%u bitmap capacity:%u blocks \n", s_blockSize, maxBlocks);
    }

    void addMemoryRegion(const void* baseAddress, uint64_t size)
    {
        if ((uint64_t)baseAddress % s_blockSize != 0) {
            logMsg(errLog, "PMM: base_address not aligned");
            return;
        }

        uint64_t blockIndx = (uint64_t)baseAddress / s_blockSize; // Convert memory address to blocks
        uint64_t endIndx = blockIndx + size / s_blockSize;

        if (endIndx > maxBlocks) {
            logMsg(errLog, "PMM: memory above %u blocks is ignored\n", maxBlocks);
            endIndx = maxBlocks;
        }

        logMsg(infLog, "PMM: MemoryRegion add: start_indx: %u num_blocks:%u ", blockIndx, endIndx - blockIndx);

        for (; blockIndx < endIndx; blockIndx++) {
            if (testBlock(blockIndx)) {
                unsetBlock(blockIndx);
                s_freeBlocks++;
            }
        }

        if (endIndx > s_maxBlocks)
            s_maxBlocks = endIndx;

        logMsg(infLog, " freeBlocks:%u \n", s_freeBlocks);
    }

    void removeMemoryRegion(const void* baseAddress, uint64_t size)
    {
        // Cover every block the region touches, even if it is not block aligned
        uint64_t blockIndx = (uint64_t)baseAddress / s_blockSize;
        uint64_t endIndx = ((uint64_t)baseAddress + size + s_blockSize - 1) / s_blockSize;

        // Blocks above maxBlocks were never added, so they are already in use
        if (endIndx > s_maxBlocks)
            endIndx = s_maxBlocks;

        logMsg(infLog, "PMM: MemoryRegion remove: start_indx: %u end_indx:%u ", blockIndx, endIndx);

        for (; blockIndx < endIndx; blockIndx++) {
            if (!testBlock(blockIndx)) {
                setBlock(blockIndx);
                s_freeBlocks--;
            }
        }

        logMsg(infLog, " freeBlocks:%u \n", s_freeBlocks);
    }

    void* allocateBlock(uint64_t numBlocks)
    {
        logMsg(infLog, "PMM: allocateBlock: num_blocks:%u \n", numBlocks);

        // If # of free blocks left is not enough, return
        if (numBlocks == 0 || s_freeBlocks < numBlocks)
            return nullptr;

        int64_t startingBlock = firstFreeBlock(numBlocks);
        logMsg(dbgLog, "PMM: firstFreeBlock starting_block : %d \n", startingBlock);

        if (startingBlock < 0) {
            return nullptr; // Couldn't find that many blocks in a row to allocate
        }

        // Found free blocks, set them as used
        for (uint64_t i = 0; i < numBlocks; i++)
            setBlock(startingBlock + i);

        s_freeBlocks -= numBlocks;

        // Convert blocks to bytes to get start of actual RAM that is now allocated
        uint64_t address = startingBlock * s_blockSize;

        logMsg(dbgLog, "PMM: allocateBlock: freeBlocks: %u  address:0x%llx \n", s_freeBlocks, address);
        return reinterpret_cast<void*>(address); // Physical memory location of allocated blocks
    }

    void freeBlocks(const void* addr, uint64_t numBlocks)
    {
        if ((uint64_t)addr % s_blockSize != 0) {
            logMsg(errLog, "PMM: freeBlocks: address 0x%llx not aligned\n", (uint64_t)addr);
            return;
        }

        uint64_t startingBlock = (uint64_t)addr / s_blockSize; // Convert address to blocks

        for (uint64_t i = 0; i < numBlocks; i++) {
            uint64_t block = startingBlock + i;

            // Freeing a block that is not allocated would corrupt freeBlocks
            if (block >= s_maxBlocks || !testBlock(block)) {
                logMsg(errLog, "PMM: freeBlocks: block %u is not allocated\n", block);
                continue;
            }

            unsetBlock(block);
            s_freeBlocks++;
        }
    }

    uint64_t getMaxBlockSize()
    {
        return s_maxBlocks / 8;
    }

    uint64_t getFreeBlockCount()
    {
        return s_freeBlocks;
    }

    // ------------------------------------------------------------
    // 4 KB frame allocator
    // ------------------------------------------------------------

    static constexpr uint64_t framesPerBlock = pageSize2Mb / frameSize4Kb;

    /**
     * @brief Maximum number of 2 MB blocks split into 4 KB frames.
     * 16 pools give 8192 frames, enough for 8192 page tables.
     */
    static constexpr uint32_t maxFramePools = 16;

    /**
     * @brief A 2 MB block split into 4 KB frames
     */
    struct FramePool {
        uint64_t base; ///< Physical address of the 2 MB block
        uint64_t bitmap[framesPerBlock / 64]; ///< 1 = frame in use
        uint32_t used; ///< Number of frames in use
    };

    static FramePool s_framePools[maxFramePools];
    static uint32_t s_framePoolCount {};

    static void* takeFrame(FramePool& pool)
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

    void* allocateFrame()
    {
        for (uint32_t i = 0; i < s_framePoolCount; i++) {
            if (s_framePools[i].used < framesPerBlock)
                return takeFrame(s_framePools[i]);
        }

        if (s_framePoolCount == maxFramePools) {
            logMsg(errLog, "PMM: allocateFrame: all %u frame pools are full\n", maxFramePools);
            return nullptr;
        }

        void* block = allocateBlock(1);
        if (block == nullptr)
            return nullptr;

        FramePool& pool = s_framePools[s_framePoolCount++];
        memset(&pool, 0, sizeof(pool));
        pool.base = reinterpret_cast<uint64_t>(block);

        logMsg(dbgLog, "PMM: allocateFrame: new frame pool at 0x%llx \n", pool.base);
        return takeFrame(pool);
    }

    void freeFrame(const void* addr)
    {
        uint64_t address = reinterpret_cast<uint64_t>(addr);

        for (uint32_t i = 0; i < s_framePoolCount; i++) {
            FramePool& pool = s_framePools[i];
            if (address < pool.base || address >= pool.base + pageSize2Mb)
                continue;

            uint64_t frame = (address - pool.base) / frameSize4Kb;
            uint64_t mask = 1ULL << (frame % 64);

            if (address % frameSize4Kb != 0 || !(pool.bitmap[frame / 64] & mask)) {
                logMsg(errLog, "PMM: freeFrame: frame 0x%llx is not allocated\n", address);
                return;
            }

            pool.bitmap[frame / 64] &= ~mask;
            pool.used--;

            // Give an empty pool back to the block allocator
            if (pool.used == 0) {
                freeBlocks(reinterpret_cast<void*>(pool.base), 1);
                s_framePools[i] = s_framePools[--s_framePoolCount];
            }
            return;
        }

        logMsg(errLog, "PMM: freeFrame: 0x%llx does not belong to any frame pool\n", address);
    }

}; // namespace PhysicalMemory
}; // namespace Memory
