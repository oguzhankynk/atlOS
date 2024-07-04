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
#include <Memory/Memory.hpp>
#include <stdio.h>
#include <string.h>

namespace PhysicalMemory
{

    static uint32_t blockSize{};
    static uint32_t *memory{};
    static uint32_t maxBlocks{};
    static uint32_t usedBlocks{};

    static int32_t FirstFreeBlock(uint32_t num_blocks)
    {
        if (num_blocks == 0)
            return -1; // Can't return no memory, error

        // Test 32 blocks at a time
        for (uint32_t i = 0; i < maxBlocks / 32; i++)
        {
            if (memory[i] != 0xFFFFFFFF)
            {
                // At least 1 bit is not set within this 32bit chunk of memory,
                //   find that bit by testing each bit
                for (int32_t j = 0; j < 32; j++)
                {
                    int32_t bit = 1 << j;

                    // If bit is unset/0, found start of a free region of memory
                    if (!(memory[i] & bit))
                    {
                        uint32_t free_blocks = 0;

                        for (uint32_t count = 0; count < num_blocks; count++)
                        {
                            if ((j + count > 31) && (i + 1 <= maxBlocks / 32))
                            {
                                if (!(memory[i + 1] & (1 << ((j + count) - 32))))
                                    free_blocks++;
                            }
                            else
                            {
                                if (!(memory[i] & (1 << (j + count))))
                                    free_blocks++;
                            }

                            if (free_blocks == num_blocks) // Found enough free space
                                return i * 32 + j;
                        }
                    }
                }
            }
        }

        return -1; // No free region of memory large enough
    }

    static void SetBlock(uint32_t bit)
    {
        memory[bit / 32] |= (1 << (bit % 32));
    }

    static void UnsetBlock(uint32_t bit)
    {
        memory[bit / 32] &= ~(1 << (bit % 32));
    }

    void Init(const void *start_address, uint32_t physical_memory_size, uint32_t block_size)
    {
        blockSize = block_size;
        memory = (uint32_t *)start_address;
        maxBlocks = physical_memory_size / blockSize;
        usedBlocks = maxBlocks; // To start off, every block will be "used/reserved"

        // By default, set all memory in use (used blocks/bits = 1, every block is set)
        // Each byte of memory map holds 8 bits/blocks
        memset(memory, 0xFF, maxBlocks / 8);

    }

    void AddMemoryRegion(const void *base_address, uint32_t size)
    {
        uint32_t align = (uint32_t)base_address / blockSize; // Convert memory address to blocks
        uint32_t num_blocks = size / blockSize;              // Convert size to blocks

        for (; num_blocks > 0; num_blocks--)
        {
            UnsetBlock(align++);
            usedBlocks--;
        }
    }

    void RemoveMemoryRegion(const void *base_address, uint32_t size)
    {
        int32_t align = (uint32_t)base_address / blockSize; // Convert memory address to blocks
        int32_t num_blocks = size / blockSize;              // Convert size to blocks

        for (; num_blocks > 0; num_blocks--)
        {
            SetBlock(align++);
            usedBlocks++;
        }
    }

    void *AllocateBlock(uint32_t num_blocks)
    {
        // If # of free blocks left is not enough, we can't allocate any more, return
        if ((maxBlocks - usedBlocks) <= num_blocks)
            return 0;

        int32_t starting_block = FirstFreeBlock(num_blocks);
        if (starting_block == -1)
            return 0; // Couldn't find that many blocks in a row to allocate

        // Found free blocks, set them as used
        for (uint32_t i = 0; i < num_blocks; i++)
            SetBlock(starting_block + i);

        usedBlocks += num_blocks; // Blocks are now used/reserved, increase count

        // Convert blocks to bytes to get start of actual RAM that is now allocated
        uint32_t address = starting_block * blockSize;

        return (uint32_t *)address; // Physical memory location of allocated blocks
    }

    void FreeBlocks(const void *addr, uint32_t num_blocks)
    {
        int32_t starting_block = (uint32_t)addr / blockSize; // Convert address to blocks

        for (uint32_t i = 0; i < num_blocks; i++)
            UnsetBlock(starting_block + i); // Unset bits/blocks in memory map, to free

        usedBlocks -= num_blocks; // Decrease used block count
    }

    uint32_t GetMaxBlockSize()
    {
        return maxBlocks / 8;
    }

}; // namespace PhysicalMemory