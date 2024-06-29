/**
 * @file PhysicalMemory.hpp
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

#include "stdint.h"

/**
 * @brief
 *
 */
namespace PhysicalMemory
{

    void Init(const void *start_address, uint32_t size);
    void AddMemoryRegion(const void *base_address, uint32_t size);
    void RemoveMemoryRegion(const void *base_address, uint32_t size);
    void *AllocateBlock(uint32_t num_blocks);
    void FreeBlocks(const void *addr, uint32_t num_blocks);
    uint32_t GetMaxBlockSize();

}; // namespace PhysicalMemory

#endif /* D83E69E7_DE4E_42CB_A05E_FA455D9BAF9F */
