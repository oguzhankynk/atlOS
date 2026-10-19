/**
 * @file pmm.hpp
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
#pragma once

#include <mm/constants.hpp>
#include <stdint.h>

namespace Memory {

/**
 * @brief Manages physical memory in 2 MB blocks, plus 4 KB frames for page tables.
 *
 * Only the bookkeeping lives here: the managed memory itself is never read or
 * written, so an instance can describe any layout (the self tests build small
 * ones). Addresses are physical.
 *
 * Block 0 (physical address 0) is never handed out, because its address would
 * look like the nullptr that signals failure.
 */
class PhysicalMemoryManager {
public:
    static constexpr uint64_t blockSize = pageSize2Mb;

    /// Bitmap bytes needed to describe memorySize bytes of physical memory
    static constexpr uint64_t bitmapBytesFor(uint64_t memorySize)
    {
        return (memorySize / blockSize + 7) / 8;
    }

    /**
     * @param bitmap Storage for one bit per block, owned by the caller and kept
     *               alive as long as the manager. Capacity is bitmapBytes * 8 blocks.
     *
     * Every block starts in use; addRegion() makes memory available.
     */
    PhysicalMemoryManager(uint8_t* bitmap, uint64_t bitmapBytes);

    PhysicalMemoryManager(const PhysicalMemoryManager&) = delete;
    PhysicalMemoryManager& operator=(const PhysicalMemoryManager&) = delete;

    /**
     * @brief Make the whole blocks of [base, base + size) available.
     * base must be block aligned; blocks beyond the capacity are ignored.
     */
    void addRegion(const void* base, uint64_t size);

    /**
     * @brief Mark every block touched by [base, base + size) as in use (it need
     * not be aligned).
     */
    void reserveRegion(const void* base, uint64_t size);

    /**
     * @brief Allocate count contiguous blocks.
     * @return Physical address of the first block, or nullptr.
     */
    void* allocateBlocks(uint64_t count);

    /**
     * @brief Free blocks returned by allocateBlocks. Unaligned addresses and
     * blocks that are not allocated are rejected with an error message.
     */
    void freeBlocks(const void* base, uint64_t count);

    uint64_t freeBlockCount() const { return m_freeBlocks; }

    /// Number of blocks the bitmap can describe
    uint64_t capacity() const { return m_capacity; }

    /**
     * @brief Allocate a single 4 KB physical frame (e.g. for a page table).
     * Frames are cut from 2 MB blocks taken from this manager.
     * @return Physical address of the frame, or nullptr if out of memory.
     */
    void* allocateFrame();

    /// Free a frame returned by allocateFrame; an empty pool gives its block back.
    void freeFrame(const void* frame);

    /// 16 pools of 512 frames, enough for 8192 page tables
    static constexpr uint32_t maxFramePools = 16;
    static constexpr uint64_t framesPerBlock = pageSize2Mb / frameSize4Kb;

private:
    /// A 2 MB block split into 4 KB frames
    struct FramePool {
        uint64_t base; ///< Physical address of the 2 MB block
        uint64_t bitmap[framesPerBlock / 64]; ///< 1 = frame in use
        uint32_t used; ///< Number of frames in use
    };

    bool testBlock(uint64_t block) const;
    void setBlock(uint64_t block);
    void unsetBlock(uint64_t block);
    int64_t firstFreeBlock(uint64_t count) const;
    static void* takeFrame(FramePool& pool);

    uint8_t* m_bitmap;
    uint64_t m_capacity; ///< blocks the bitmap can describe
    uint64_t m_maxBlocks {}; ///< one past the highest added block; the scan stops here
    uint64_t m_freeBlocks {};

    FramePool m_framePools[maxFramePools] {};
    uint32_t m_framePoolCount {};
};

} // namespace Memory
