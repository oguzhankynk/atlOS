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
        static uint64_t usedBlocks{};

        static inline void SetBlock(uint64_t bit)
        {
            bitmap[bit / 8] |= (1 << (bit % 8));
        }

        static inline void UnsetBlock(uint64_t bit)
        {
            bitmap[bit / 8] &= ~(1 << (bit % 8));
        }

        static int64_t FirstFreeBlock(uint64_t num_blocks)
        {
            LOG_MSG(INF_LOG, "PMM: FirstFreeBlock: number of requested blocks:%u free block:%u \n", num_blocks, maxBlocks - usedBlocks);

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
            maxBlocks = MAX_MEMORY_SIZE / blockSize;
            usedBlocks = maxBlocks;

            LOG_MSG(INF_LOG, "PMM: Init: blockSize:%u maxBlocks:%u usedBlocks:%u \n", blockSize, maxBlocks, usedBlocks);
        }

        void AddMemoryRegion(const void *base_address, uint64_t size)
        {
            if ((uint64_t)base_address % blockSize != 0)
            {
                LOG_MSG(ERR_LOG, "PMM: base_address not aligned");
                return;
            }

            uint64_t block_indx = (uint64_t)base_address / blockSize; // Convert memory address to blocks
            uint64_t num_blocks = size / blockSize;                   // Convert size to blocks

            LOG_MSG(INF_LOG, "PMM: MemoryRegion add: start_indx: %u num_blocks:%u ", block_indx, num_blocks);

            for (; num_blocks > 0; num_blocks--)
            {
                UnsetBlock(block_indx);
                block_indx++;
                usedBlocks--;
            }

            LOG_MSG(INF_LOG, " usedBlocks:%u \n", usedBlocks);
        }

        void RemoveMemoryRegion(const void *base_address, uint64_t size)
        {
            int64_t block_indx = (uint64_t)base_address / blockSize; // Convert memory address to blocks
            int64_t num_blocks = (size + blockSize - 1) / blockSize; // Convert size to blocks

            LOG_MSG(INF_LOG, "PMM: MemoryRegion remove: start_indx: %u num_blocks:%u ", block_indx, num_blocks);

            for (; num_blocks > 0; num_blocks--)
            {
                SetBlock(block_indx);
                block_indx++;
                usedBlocks++;
            }

            LOG_MSG(INF_LOG, " usedBlocks:%u \n", usedBlocks);
        }

        void *AllocateBlock(uint64_t num_blocks)
        {
            LOG_MSG(INF_LOG, "PMM: AllocateBlock: num_blocks:%u \n", num_blocks);

            // If # of free blocks left is not enough, return
            if ((maxBlocks - usedBlocks) <= num_blocks)
                return nullptr;

            uint64_t starting_block = FirstFreeBlock(num_blocks);
            LOG_MSG(DBG_LOG, "PMM: FirstFreeBlock starting_block : %d \n", starting_block);

            if (starting_block == -1)
            {
                return nullptr; // Couldn't find that many blocks in a row to allocate
            }

            // Found free blocks, set them as used
            for (uint64_t i = 0; i < num_blocks; i++)
                SetBlock(starting_block + i);

            usedBlocks += num_blocks; // Blocks are now used/reserved, increase count

            // Convert blocks to bytes to get start of actual RAM that is now allocated
            uint64_t address = starting_block * blockSize;

            LOG_MSG(DBG_LOG, "PMM: AllocateBlock: usedBlocks: %u  address:%u \n", usedBlocks, address);
            return reinterpret_cast<void *>(address); // Physical memory location of allocated blocks
        }

        void FreeBlocks(const void *addr, uint64_t num_blocks)
        {
            int64_t starting_block = (uint64_t)addr / blockSize; // Convert address to blocks

            // printf("PMM: FreeBlocks: starting block indx:%u adr:%u number of blocks:%u \n", starting_block, (uint64_t )addr,  num_blocks);

            for (uint64_t i = 0; i < num_blocks; i++)
                UnsetBlock(starting_block + i); // Unset bits/blocks in memory map, to free

            usedBlocks -= num_blocks; // Decrease used block count
        }

        uint64_t GetMaxBlockSize()
        {
            return maxBlocks / 8;
        }

    }; // namespace PhysicalMemory
}; // namespace Memory