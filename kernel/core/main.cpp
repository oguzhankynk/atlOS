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

#include <inttypes.h>
#include <stdio.h>
#include <string.h>

#include <core/banner.hpp>
#include <drivers/tty.hpp>

#include <mm/constants.hpp>
#include <mm/heap.hpp>
#include <mm/memory.hpp>
#include <mm/pmm.hpp>
#include <mm/vmm.hpp>

#include <dev/device_manager.hpp>
#include <drivers/serial.hpp>

#include <arch/x86_64/boot/multiboot2.inc>
#include <arch/x86_64/idt/idt.hpp>

#include <cxx/init.hpp>

#include <sched/scheduler.hpp>
#include <sched/thread.hpp>

#include <arch/x86_64/idt/handlers.hpp>
#include <arch/x86_64/io/ioport.hpp>
#include <cxx/uninitialized.hpp>
#include <drivers/pic.hpp>
#include <drivers/pit.hpp>

#include <tests/selftest.hpp>

extern void threadTaskA(Threading::Scheduler& scheduler);
extern void threadTaskB(Threading::Scheduler& scheduler);

// Objects owned by kernelMain, the composition root. They are constructed
// explicitly below, in dependency order, and handed to their users by
// reference; nothing else reaches them through a global.
static Cxx::Uninitialized<Memory::PhysicalMemoryManager> s_pmm;
static Cxx::Uninitialized<Memory::AddressSpace> s_kernelSpace;
static Cxx::Uninitialized<Drivers::PIC> s_pic;
static Cxx::Uninitialized<Drivers::PIT> s_pit;
static Cxx::Uninitialized<Threading::Scheduler> s_scheduler;

// One bit per 2 MB block, enough for 256 GB of physical memory
static uint8_t s_pmmBitmap[Memory::PhysicalMemoryManager::bitmapBytesFor(256ULL * 1024 * 1024 * 1024)];

/**
 * @brief entry point of atlOS
 *
 */
extern "C" void kernelMain(uint64_t mbInfo, uint32_t mbMagic)
{

    // Initialize vga terminal
    terminalInitialize();

    // Set the write handler to vga terminalWrite for early debugging output
    set_write_handler(terminalWrite);

    // Initialize serial port
    int statSerialInit = SerialPort::init();

    printf("stat_serial_init: %d \n", statSerialInit);

    // Set the write handler to SerialPort::writeSerial for later debugging output
    set_write_handler(SerialPort::writeSerial);

    // Print the banner
    atlOSBanner();

    // Call global constructors
    Cxx::callConstructors();

    // Initialize IDT
    IDT::init();

    // Validate multiboot information
    if (mbMagic != MULTIBOOT2_BOOTLOADER_MAGIC) {
        printf("Invalid magic number: 0x%x\n", mbMagic);
        return;
    } else {
        printf("Valid magic number: 0x%x\n", mbMagic);
    }

    // Memory: physical blocks from the multiboot memory map, then the VMM on top
    Memory::printLayout();
    Memory::PhysicalMemoryManager& pmm = s_pmm.construct(s_pmmBitmap, sizeof(s_pmmBitmap));
    Memory::populateFromMultiboot(pmm, mbInfo);
    Memory::AddressSpace& kernelSpace = s_kernelSpace.construct(pmm);

    // kmalloc gets its pages from the kernel address space (liballoc hooks)
    Memory::setHeapAddressSpace(kernelSpace);

    DeviceManager::init();

    // Interrupt controller: the 8259 PIC (there is no APIC driver yet). The ISR
    // dispatcher is the one place that needs it without being handed it.
    Drivers::PIC& pic = s_pic.construct();
    Handlers::setInterruptController(&pic);

    // Scheduler tick: the PIT, delivered through the PIC
    Drivers::PIT& pit = s_pit.construct(pic);

    IOPort::enableInterrupts();

    // The boot flow becomes the scheduler's first task
    Threading::Scheduler& scheduler = s_scheduler.construct(pit);

#ifdef ATLOS_SELFTEST
    int failed = SelfTest::run(pmm, kernelSpace, scheduler);
    SelfTest::exitQemu(failed == 0 ? 0 : 1);

    // Only reached when QEMU has no isa-debug-exit device
    while (1) {
        asm volatile("cli; hlt");
    }
#endif

    printf("Creating threads...\n");
    Threading::Thread t1(scheduler, kernelSpace, threadTaskA);
    Threading::Thread t2(scheduler, kernelSpace, threadTaskB);

    printf("Threads running...\n");
    while (1) {
        scheduler.yield();
    }
}
