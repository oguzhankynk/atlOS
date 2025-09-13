/**
 * @file kernel.cpp
 * @author Oguzhan KAYNAK
 * @brief Kernel implementation file containing kernel initialization and utility functions.
 *
 * This file contains the implementation of kernel initialization and various utility functions
 * used by the kernel during runtime. It includes functions for hardware initialization,
 * interrupt handling, memory management, and other essential operations.
 * Additionally, it may contain device drivers and other kernel-specific functionalities.
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

#include <stdio.h>
#include <inttypes.h>

#include <kernel/tty.h>
#include <kernel/banner.h>

#include <Memory/Memory.hpp>
#include <Memory/MemoryConstants.hpp>

#include <kernel/sys/multiboot2.h>

#include <cxx/init.hpp>

struct myStruct
{
    void print()
    {
        printf("MyStruct::print() called\n");
    }
    myStruct(const char *str)
    {
        printf("MyStruct constructor called:%s\n", str);
    }
};
struct myStruct2
{
    int x;
};
struct myStruct3
{
    int x{4};
};

myStruct glob{"global1"};
myStruct glob2{"global2"};

/**
 * @brief entry point of atlOS
 *
 */
extern "C" void kernelMain(uint64_t mbInfo, uint32_t mbMagic)
{

    terminal_initialize();

    atlOSBanner();

    cxx::callConstructors();

    myStruct local1{"local1"};

    if (mbMagic != MULTIBOOT2_BOOTLOADER_MAGIC)
    {
        printf("Invalid magic number: 0x%x\n", mbMagic);
        return;
    }
    else
    {
        printf("Valid magic number: 0x%x\n", mbMagic);
    }

    Memory::Init(mbInfo);

    while (1)
        ;
    return;
}
