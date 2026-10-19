/**
 * @file main.cpp
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

#include <drivers/tty.hpp>
#include <core/banner.hpp>

#include <mm/memory.hpp>
#include <mm/constants.hpp>
#include <mm/pmm.hpp>
#include <mm/vmm.hpp>
#include <mm/heap.hpp>

#include <dev/device_manager.hpp>
#include <drivers/serial.hpp>

#include <arch/x86_64/boot/multiboot2.inc>
#include <arch/x86_64/idt/idt.hpp>

#include <cxx/init.hpp>

#include <sched/scheduler.hpp>
#include <sched/thread.hpp>

#include <drivers/pic.hpp>

#include <tests/selftest.hpp>

extern void threadTaskA();
extern void threadTaskB();

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

    // The scheduler tick comes from the PIT through the 8259 PIC (there is no
    // APIC driver yet). Init also enables interrupts.
    PIC::Init();
    Scheduler::Init();

#ifdef ATLOS_SELFTEST
    int failed = SelfTest::Run();
    SelfTest::ExitQemu(failed == 0 ? 0 : 1);

    // Only reached when QEMU has no isa-debug-exit device
    while (1)
    {
        asm volatile("cli; hlt");
    }
#endif

    printf("Creating threads...\n");
    Threading::Thread t1(threadTaskA);
    Threading::Thread t2(threadTaskB);

    printf("Threads running...\n");
    while (1)
    {
        Scheduler::yield();
    }
}
