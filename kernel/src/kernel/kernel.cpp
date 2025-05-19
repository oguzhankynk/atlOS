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
#include <string.h>

#include <arch/i386/GDT/gdt.hpp>
#include <arch/i386/IDT/idt.hpp>

#include <kernel/tty.h>
#include <kernel/BIOS/BIOS.h>
#include <kernel/sys/multiboot.h>
#include <kernel/sys/IOPort.hpp>
#include <kernel/panic.h>
#include <kernel/banner.h>

#include <Device/DeviceManager.hpp>
#include <Memory/Memory.hpp>
#include <Memory/PhysicalMemory.hpp>
#include <Memory/KHeap.h>

#include <Schduler/schduler.hpp>

/**
 * @brief entry point of atlOS
 *
 */
extern "C" void kernelMain(uint32_t multiboot_bootloader_magic, uint32_t multiboot_info_addr)
{

    terminal_initialize();

    atlOSBanner();

    GDT::Init();

    IDT::Init();

    if (multiboot_bootloader_magic != MULTIBOOT_BOOTLOADER_MAGIC)
    {
        kernel_panic("magic number is invalid.");
    }

    Memory::Init((multiboot_info_t *)multiboot_info_addr);

    DeviceManager::Init();

    // Schedular::Init();

    printf("end of kernel \n");

    while (1)
    {
        asm volatile("hlt");
    }
}
