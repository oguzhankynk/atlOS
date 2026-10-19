/**
 * @file PhysicalMemory.cpp
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


#include <Memory/PhysicalMemory.hpp>

#include <string.h>

#include <kernel/Logger.hpp>
#include <Memory/Memory.hpp>
#include <Memory/MemoryConstants.hpp>

namespace Memory
{
    namespace PhysicalMemory
    {
        /// Enable / Disable logs
        static constexpr LogFlag DBG_LOG = LogFlag::Log;
        static constexpr LogFlag INF_LOG = LogFlag::Log;
        static constexpr LogFlag ERR_LOG = LogFlag::Log;

        /**
         * @brief Maximum physical memoryy size (byte)
         *
         */
        static constexpr uint64_t MAX_MEMORY_SIZE = 256ULL * 1024ULL * 1024ULL * 1024ULL;

        /**
         * @brief Maximum count of physical pages
         *
         */
        static constexpr uint64_t MAX_BLOCKS = MAX_MEMORY_SIZE / PAGE_SIZE_2MB;

        /**
         * @brief Physical memory bitmap size (byte)
         *
         */
        static constexpr uint64_t BITMAP_SIZE = (MAX_BLOCKS + 7) / 8;

        /**
         * @brief Bitmap for manege physical memory
         *
         */
        static uint8_t bitmap[BITMAP_SIZE];

        static uint64_t blockSize{};
        
        static uint64_t maxBlocks{};

        static uint64_t freeBlocks{};

        static inline void SetBlock(uint64_t bit)
        {
            bitmap[bit / 8] |= (1 << (bit % 8));
        }

        static inline void UnsetBlock(uint64_t bit)
        {
            bitmap[bit / 8] &= ~(1 << (bit % 8));
        }

        static inline bool TestBlock(uint64_t bit)
        {
            return bitmap[bit / 8] & (1 << (bit % 8));
        }

        static int64_t FirstFreeBlock(uint64_t num_blocks)
        {
            LOG_MSG(INF_LOG, "PMM: FirstFreeBlock: number of requested blocks:%u free block:%u \n", num_blocks, freeBlocks);

            if (num_blocks == 0)
                return -1; // Can't return no memory, error

            uint64_t consecutive_free = 0; // Counter for consecutive 0 bits
            uint64_t start_index = 0;      // Index of the start of the free block series

            for (uint64_t bit_index = 0; bit_index < maxBlocks; bit_index++)
            {
                uint64_t byte_index = bit_index / 8;
                uint8_t bit_offset = bit_index % 8;

                // Check if the current bit is 0
                if (!(bitmap[byte_index] & (1 << bit_offset)))
                {
                    if (consecutive_free == 0)
                    {
                        LOG_MSG(DBG_LOG, "PMM: FirstFreeBlock: the current bit is 0 : %u \n", bit_index);
                        start_index = bit_index; // Save the start index of the series
                    }
                    consecutive_free++;

                    // If a series of sufficient length is found
                    if (consecutive_free == num_blocks)
                    {
                        return start_index;
                    }
                }
                else
                {
                    if (consecutive_free != 0)
                        LOG_MSG(DBG_LOG, "PMM: FirstFreeBlock: the current bit is 1 : %u \n", consecutive_free);

                    consecutive_free = 0; // Reset if a 1 bit is encountered
                }
            }

            return -1; // No free region of memory large enough
        }

        void Init()
        {
            // set all memory in use
            memset(bitmap, 0xFF, sizeof(bitmap));

            blockSize = PAGE_SIZE_2MB;
            maxBlocks = 0;
            freeBlocks = 0;

            LOG_MSG(INF_LOG, "PMM: Init: blockSize:%u bitmap capacity:%u blocks \n", blockSize, MAX_BLOCKS);
        }

        void AddMemoryRegion(const void *base_address, uint64_t size)
        {
            if ((uint64_t)base_address % blockSize != 0)
            {
                LOG_MSG(ERR_LOG, "PMM: base_address not aligned");
                return;
            }

            uint64_t block_indx = (uint64_t)base_address / blockSize; // Convert memory address to blocks
            uint64_t end_indx = block_indx + size / blockSize;

            if (end_indx > MAX_BLOCKS)
            {
                LOG_MSG(ERR_LOG, "PMM: memory above %u blocks is ignored\n", MAX_BLOCKS);
                end_indx = MAX_BLOCKS;
            }

            LOG_MSG(INF_LOG, "PMM: MemoryRegion add: start_indx: %u num_blocks:%u ", block_indx, end_indx - block_indx);

            for (; block_indx < end_indx; block_indx++)
            {
                if (TestBlock(block_indx))
                {
                    UnsetBlock(block_indx);
                    freeBlocks++;
                }
            }

            if (end_indx > maxBlocks)
                maxBlocks = end_indx;

            LOG_MSG(INF_LOG, " freeBlocks:%u \n", freeBlocks);
        }

        void RemoveMemoryRegion(const void *base_address, uint64_t size)
        {
            // Cover every block the region touches, even if it is not block aligned
            uint64_t block_indx = (uint64_t)base_address / blockSize;
            uint64_t end_indx = ((uint64_t)base_address + size + blockSize - 1) / blockSize;

            // Blocks above maxBlocks were never added, so they are already in use
            if (end_indx > maxBlocks)
                end_indx = maxBlocks;

            LOG_MSG(INF_LOG, "PMM: MemoryRegion remove: start_indx: %u end_indx:%u ", block_indx, end_indx);

            for (; block_indx < end_indx; block_indx++)
            {
                if (!TestBlock(block_indx))
                {
                    SetBlock(block_indx);
                    freeBlocks--;
                }
            }

            LOG_MSG(INF_LOG, " freeBlocks:%u \n", freeBlocks);
        }

        void *AllocateBlock(uint64_t num_blocks)
        {
            LOG_MSG(INF_LOG, "PMM: AllocateBlock: num_blocks:%u \n", num_blocks);

            // If # of free blocks left is not enough, return
            if (num_blocks == 0 || freeBlocks < num_blocks)
                return nullptr;

            int64_t starting_block = FirstFreeBlock(num_blocks);
            LOG_MSG(DBG_LOG, "PMM: FirstFreeBlock starting_block : %d \n", starting_block);

            if (starting_block < 0)
            {
                return nullptr; // Couldn't find that many blocks in a row to allocate
            }

            // Found free blocks, set them as used
            for (uint64_t i = 0; i < num_blocks; i++)
                SetBlock(starting_block + i);

            freeBlocks -= num_blocks;

            // Convert blocks to bytes to get start of actual RAM that is now allocated
            uint64_t address = starting_block * blockSize;

            LOG_MSG(DBG_LOG, "PMM: AllocateBlock: freeBlocks: %u  address:0x%llx \n", freeBlocks, address);
            return reinterpret_cast<void *>(address); // Physical memory location of allocated blocks
        }

        void FreeBlocks(const void *addr, uint64_t num_blocks)
        {
            if ((uint64_t)addr % blockSize != 0)
            {
                LOG_MSG(ERR_LOG, "PMM: FreeBlocks: address 0x%llx not aligned\n", (uint64_t)addr);
                return;
            }

            uint64_t starting_block = (uint64_t)addr / blockSize; // Convert address to blocks

            for (uint64_t i = 0; i < num_blocks; i++)
            {
                uint64_t block = starting_block + i;

                // Freeing a block that is not allocated would corrupt freeBlocks
                if (block >= maxBlocks || !TestBlock(block))
                {
                    LOG_MSG(ERR_LOG, "PMM: FreeBlocks: block %u is not allocated\n", block);
                    continue;
                }

                UnsetBlock(block);
                freeBlocks++;
            }
        }

        uint64_t GetMaxBlockSize()
        {
            return maxBlocks / 8;
        }

        uint64_t GetFreeBlockCount()
        {
            return freeBlocks;
        }

        // ------------------------------------------------------------
        // 4 KB frame allocator
        // ------------------------------------------------------------

        static constexpr uint64_t FRAMES_PER_BLOCK = PAGE_SIZE_2MB / FRAME_SIZE_4KB;

        /**
         * @brief Maximum number of 2 MB blocks split into 4 KB frames.
         * 16 pools give 8192 frames, enough for 8192 page tables.
         */
        static constexpr uint32_t MAX_FRAME_POOLS = 16;

        /**
         * @brief A 2 MB block split into 4 KB frames
         */
        struct FramePool
        {
            uint64_t base;                         ///< Physical address of the 2 MB block
            uint64_t bitmap[FRAMES_PER_BLOCK / 64]; ///< 1 = frame in use
            uint32_t used;                         ///< Number of frames in use
        };

        static FramePool framePools[MAX_FRAME_POOLS];
        static uint32_t framePoolCount{};

        static void *TakeFrame(FramePool &pool)
        {
            for (uint64_t word = 0; word < FRAMES_PER_BLOCK / 64; word++)
            {
                if (pool.bitmap[word] == ~0ULL)
                    continue;

                uint64_t bit = __builtin_ctzll(~pool.bitmap[word]);
                pool.bitmap[word] |= (1ULL << bit);
                pool.used++;

                return reinterpret_cast<void *>(pool.base + (word * 64 + bit) * FRAME_SIZE_4KB);
            }

            return nullptr;
        }

        void *AllocateFrame()
        {
            for (uint32_t i = 0; i < framePoolCount; i++)
            {
                if (framePools[i].used < FRAMES_PER_BLOCK)
                    return TakeFrame(framePools[i]);
            }

            if (framePoolCount == MAX_FRAME_POOLS)
            {
                LOG_MSG(ERR_LOG, "PMM: AllocateFrame: all %u frame pools are full\n", MAX_FRAME_POOLS);
                return nullptr;
            }

            void *block = AllocateBlock(1);
            if (block == nullptr)
                return nullptr;

            FramePool &pool = framePools[framePoolCount++];
            memset(&pool, 0, sizeof(pool));
            pool.base = reinterpret_cast<uint64_t>(block);

            LOG_MSG(DBG_LOG, "PMM: AllocateFrame: new frame pool at 0x%llx \n", pool.base);
            return TakeFrame(pool);
        }

        void FreeFrame(const void *addr)
        {
            uint64_t address = reinterpret_cast<uint64_t>(addr);

            for (uint32_t i = 0; i < framePoolCount; i++)
            {
                FramePool &pool = framePools[i];
                if (address < pool.base || address >= pool.base + PAGE_SIZE_2MB)
                    continue;

                uint64_t frame = (address - pool.base) / FRAME_SIZE_4KB;
                uint64_t mask = 1ULL << (frame % 64);

                if (address % FRAME_SIZE_4KB != 0 || !(pool.bitmap[frame / 64] & mask))
                {
                    LOG_MSG(ERR_LOG, "PMM: FreeFrame: frame 0x%llx is not allocated\n", address);
                    return;
                }

                pool.bitmap[frame / 64] &= ~mask;
                pool.used--;

                // Give an empty pool back to the block allocator
                if (pool.used == 0)
                {
                    FreeBlocks(reinterpret_cast<void *>(pool.base), 1);
                    framePools[i] = framePools[--framePoolCount];
                }
                return;
            }

            LOG_MSG(ERR_LOG, "PMM: FreeFrame: 0x%llx does not belong to any frame pool\n", address);
        }

    }; // namespace PhysicalMemory
}; // namespace Memory
