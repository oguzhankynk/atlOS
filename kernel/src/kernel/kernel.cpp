/**
 * @file kernel.cpp
 * @author Oguzhan KAYNAK
 * @brief Kernel implementation file containing kernel initialization and utility functions.
 *
 * This file contains the implementation of kernel initialization and various utility functions
 * used by the kernel during runtime. It includes functions for hardware initialization,
 * interrupt handling, memory management, and other essential operations.
 * Additionally, it may contain device drivers, system calls, and other kernel-specific functionalities.
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

#include <kernel/tty.h>
#include <kernel/BIOS/BIOS.h>
#include <kernel/sys/multiboot.h>
#include <kernel/panic.h>

#include <arch/i386/GDT/gdt.hpp>
#include <arch/i386/IDT/idt.hpp>

#include <hardware/pic8259.hpp>
#include <Memory/Memory.hpp>

#include <kernel/banner.h>
#include <Drivers/mouse.hpp>



/**
 * @brief firs point of ParsOS
 *
 */
extern "C" void kernel_main(uint32_t magic, uint32_t addr)
{

    GDT::getInstance().Init();
    IDT::getInstance().Init();
    terminal_initialize();

    ParsOSBanner();

    //PIC8259::getInstance().init();
    //mouse_init();

    //asm volatile("int $0x00");
    //if (magic != MULTIBOOT_BOOTLOADER_MAGIC)
    //{
    //    kernel_panic("magic number is invalid.");
    //}
//
    //multiboot_info_t* bootInfo = (multiboot_info_t*)addr;
    //uint32_t mod1 = *(uint32_t*)(bootInfo->mods_addr + 4);
    //uint32_t physicalAllocStart = (mod1 + 0xFFF) & ~0xFFF;
//
//
//
    //Memory::initMemory(bootInfo->mem_upper * 1024, physicalAllocStart);
    Memory::initMemory(0,0);
   
asm volatile("int $0x00");

    //BIOSInit();

    while (1)
        ;
}
