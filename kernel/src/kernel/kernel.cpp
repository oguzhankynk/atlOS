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
#include <string.h>

#include <kernel/tty.h>
#include <kernel/banner.h>

#include <Memory/Memory.hpp>
#include <Memory/MemoryConstants.hpp>
#include <Memory/PhysicalMemory.hpp>
#include <Memory/VirtualMemory.hpp>

#include <arch/x86_64/boot/multiboot2.inc>

#include <cxx/init.hpp>

#include <arch/x86_64/IDT/idt.hpp>

#include <Memory/KHeap.h>

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

void TestMemoryManager()
{
    printf("=== Memory Manager Test Start ===\n");

    // 1) Allocate 4 pages (2MB each)
    void *pages = Memory::VirtualMemory::allocatePage(4);
    printf("Allocated 4 pages at virtual address: %p\n", pages);

    // 2) Write some pattern to each 2MB page
    uint64_t *p = (uint64_t *)pages;
    for (size_t i = 0; i < 4 * 2 * 1024 * 1024 / sizeof(uint64_t); i += 1024 * 1024) // sparse writes
    {
        p[i] = 0xDEADBEEFDEADBEEF;
    }
    printf("Wrote pattern to pages\n");

    // 3) Verify the pattern
    for (size_t i = 0; i < 4 * 2 * 1024 * 1024 / sizeof(uint64_t); i += 1024 * 1024)
    {
        if (p[i] != 0xDEADBEEFDEADBEEF)
        {
            printf("Memory verification failed at index %zu\n", i);
        }
    }
    printf("Memory verification passed\n");

    // 4) Free pages
    Memory::VirtualMemory::freePage(pages, 4);
    printf("Freed 4 pages\n");

    // 5) malloc/free test
    void *ptr1 = kmalloc(1024);
    void *ptr2 = kmalloc(4096);
    printf("malloc ptr1: %p, ptr2: %p\n", ptr1, ptr2);

    memset(ptr1, 0xAA, 1024);
    memset(ptr2, 0xBB, 4096);

    kfree(ptr1);
    kfree(ptr2);
    printf("Freed malloc blocks\n");

    printf("=== Memory Manager Test End ===\n");
}

/**
 * @brief entry point of atlOS
 *
 */
extern "C" void kernelMain(uint64_t mbInfo, uint32_t mbMagic)
{

    terminal_initialize();

    atlOSBanner();

    cxx::callConstructors();

    IDT::Init();

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

    TestMemoryManager();

    while (1)
        ;
    return;
}
