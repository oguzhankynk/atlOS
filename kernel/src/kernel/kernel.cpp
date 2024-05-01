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

#include <kernel/banner.h>

/**
 * @brief firs point of ParsOS
 * 
 */
extern "C" void kernel_main(uint32_t magic, uint32_t addr)
{
    GDT::getInstance().Init();
    IDT::getInstance().Init();
	
    terminal_initialize();
    printf("HELLO WORLD FROM ParsOS\n");
    ParsOSBanner();

	if(magic != MULTIBOOT_BOOTLOADER_MAGIC)
	{
		kernel_panic("magic number is invalid.");
	}

	multiboot_info_t* mbd = (multiboot_info_t*) addr;

	if(!(mbd->flags >> 6 & 0x1)) {
        kernel_panic("invalid memory map given by GRUB bootloader");
    }


    /* Loop through the memory map and display the values */
    for(int i = 0; i < mbd->mmap_length; i += sizeof(multiboot_memory_map_t)) 
    {
        multiboot_memory_map_t* mmmt = 
            (multiboot_memory_map_t*) (mbd->mmap_addr + i);
 
        printf("Start Addr: 0x%llx | Length: 0x%llx | Size: 0x%x | Type: %u\n",
            mmmt->addr, mmmt->len, mmmt->size, mmmt->type);
 
        if(mmmt->type == MULTIBOOT_MEMORY_AVAILABLE) {
            /* 
             * Do something with this memory block!
             * BE WARNED that some of memory shown as availiable is actually 
             * actively being used by the kernel! You'll need to take that
             * into account before writing to memory!
             */
        }
    }

	BIOSInit();

	while(1);
}


