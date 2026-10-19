/**
 * @file pmm.hpp
 * @author Oguzhan KAYNAK ()
 * @brief Header file for Physical Memory operations
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
#ifndef D83E69E7_DE4E_42CB_A05E_FA455D9BAF9F
#define D83E69E7_DE4E_42CB_A05E_FA455D9BAF9F

#include <stdint.h>

namespace Memory
{
    namespace PhysicalMemory
    {

        void init();
        void addMemoryRegion(const void *baseAddress, uint64_t size);
        void removeMemoryRegion(const void *baseAddress, uint64_t size);
        void *allocateBlock(uint64_t numBlocks);
        void freeBlocks(const void *addr, uint64_t numBlocks);
        uint64_t getMaxBlockSize();

        /**
         * @brief Number of free 2 MB blocks.
         */
        uint64_t getFreeBlockCount();

        /**
         * @brief Allocate a single 4 KB physical frame (e.g. for a page table).
         *
         * @return Physical address of the frame, or nullptr if out of memory.
         */
        void *allocateFrame();

        /**
         * @brief Free a frame returned by allocateFrame.
         *
         * @param addr Physical address of the frame.
         */
        void freeFrame(const void *addr);

    }; // namespace PhysicalMemory
}; // namespace Memory

#endif /* D83E69E7_DE4E_42CB_A05E_FA455D9BAF9F */
