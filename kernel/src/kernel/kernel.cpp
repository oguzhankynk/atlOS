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
#include <Memory/KHeap.h>

#include <Device/DeviceManager.hpp>
#include <Interfaces/serial.hpp>

#include <arch/x86_64/boot/multiboot2.inc>
#include <arch/x86_64/IDT/idt.hpp>

#include <cxx/init.hpp>

#include <Schduler/schduler.hpp>
#include <Thread/thread.hpp>

extern void threadTaskA();
extern void threadTaskB();

void TestMemoryManager()
{
    printf("=== Memory Manager Test Start ===\n");

    // 1) Allocate 4 pages (2MB each)
    constexpr size_t NUM_PAGES = 4;
    void *pages = Memory::VirtualMemory::allocatePage(NUM_PAGES);
    printf("Allocated %zu pages at virtual address: %p\n", NUM_PAGES, pages);

    // 2) Write some pattern to each 2MB page
    uint64_t *base = reinterpret_cast<uint64_t *>(pages);
    constexpr uint64_t PATTERN = 0xDEADBEEFDEADBEEF;
    for (size_t page = 0; page < NUM_PAGES; ++page)
    {
        size_t index = (page * Memory::PAGE_SIZE_2MB) / sizeof(uint64_t);
        base[index] = PATTERN;
    }
    printf("Wrote pattern to pages\n");

    // 3) Verify the pattern
    for (size_t page = 0; page < NUM_PAGES; ++page)
    {
        size_t index = (page * Memory::PAGE_SIZE_2MB) / sizeof(uint64_t);
        if (base[index] != PATTERN)
        {
            printf("Memory verification failed at page %zu (addr=%p)\n",
                   page, &base[index]);
            return;
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

    // Initialize vga terminal
    terminal_initialize();

    // Set the write handler to vga terminal_write for early debugging output
    set_write_handler(terminal_write);

    // Initialize serial port
    int stat_serial_init = SerialPort::Init();

    printf("stat_serial_init: %d \n", stat_serial_init);

    // Set the write handler to SerialPort::write_serial for later debugging output
    set_write_handler(SerialPort::write_serial);

    // Print the banner
    atlOSBanner();

    // Call global constructors
    cxx::callConstructors();

    // Initialize IDT
    IDT::Init();

    // Validate multiboot information
    if (mbMagic != MULTIBOOT2_BOOTLOADER_MAGIC)
    {
        printf("Invalid magic number: 0x%x\n", mbMagic);
        return;
    }
    else
    {
        printf("Valid magic number: 0x%x\n", mbMagic);
    }

    // Initialize memory management (pmm and vmm)
    Memory::Init(mbInfo);

    DeviceManager::Init();
    TestMemoryManager();

    Schedular::Init();

    printf("Creating threads...\n");
    Threading::Thread t1(threadTaskA);
    Threading::Thread t2(threadTaskB);

    printf("Threads running...\n");
    while (1)
    {
        Schedular::yield();
    }
}
