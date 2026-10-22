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
#include <core/console.hpp>
#include <drivers/vga_console.hpp>

#include <mm/constants.hpp>
#include <mm/heap.hpp>
#include <mm/memory.hpp>
#include <mm/pmm.hpp>
#include <mm/vmm.hpp>

#include <drivers/serial.hpp>
#include <fs/storage.hpp>
#include <fs/vfs.hpp>
#include <proc/process.hpp>

#include <arch/x86_64/boot/multiboot2.inc>
#include <arch/x86_64/gdt/gdt.hpp>
#include <arch/x86_64/idt/idt.hpp>

#include <cxx/init.hpp>

#include <sched/scheduler.hpp>
#include <sched/thread.hpp>

#include <arch/x86_64/idt/handlers.hpp>
#include <arch/x86_64/io/ioport.hpp>
#include <cxx/uninitialized.hpp>
#include <drivers/character_queue.hpp>
#include <drivers/keyboard.hpp>
#include <drivers/pci/config_space.hpp>
#include <drivers/pic.hpp>
#include <drivers/pit.hpp>
#include <drivers/tee_sink.hpp>
#include <shell/interpreter.hpp>

#include <core/halt.hpp>
#include <tests/crashtest.hpp>
#include <tests/selftest.hpp>

// Objects owned by kernelMain, the composition root. They are constructed
// explicitly below, in dependency order, and handed to their users by
// reference; nothing else reaches them through a global.
static Cxx::Uninitialized<Drivers::VGATextConsole> s_vga;
static Cxx::Uninitialized<Drivers::SerialPort> s_serial;
static Cxx::Uninitialized<Memory::PhysicalMemoryManager> s_pmm;
static Cxx::Uninitialized<Memory::AddressSpace> s_kernelSpace;
static Cxx::Uninitialized<Drivers::PIC> s_pic;
static Cxx::Uninitialized<Drivers::PIT> s_pit;
static Cxx::Uninitialized<Threading::Scheduler> s_scheduler;
static Cxx::Uninitialized<Drivers::TeeSink> s_console;
static Cxx::Uninitialized<Drivers::CharacterQueue> s_input;
static Cxx::Uninitialized<Drivers::PS2Keyboard> s_keyboard;
static Cxx::Uninitialized<Shell::Interpreter> s_shell;
static Cxx::Uninitialized<Fs::Vfs> s_vfs;
static Fs::BlockDeviceList s_disks; // the disks and partitions storage found
static Drivers::PciPortConfigSpace s_pci;
static Cxx::Uninitialized<Process::Runtime> s_runtime;

// One bit per 2 MB block, enough for 256 GB of physical memory
// As much memory as the physmap covers: page tables are written through it
static uint8_t s_pmmBitmap[Memory::PhysicalMemoryManager::bitmapBytesFor(Memory::physmapSize)];

/**
 * @brief entry point of atlOS
 *
 */
extern "C" void kernelMain(uint64_t mbInfo, uint32_t mbMagic)
{

    // Console: the VGA screen, and COM1 too when there is one
    Drivers::VGATextConsole& vga = s_vga.construct();
    Core::setConsole(vga);

    Drivers::SerialPort& serial = s_serial.construct(Drivers::SerialPort::com1);
    if (serial.isPresent())
        Core::setConsole(s_console.construct(vga, serial));
    else
        printf("COM1 not found, the console stays on the screen\n");

    // The version first, so every log says what it is from
    atlOSBootLine();

    // Call global constructors
    Cxx::callConstructors();

    // TSS first: the IDT gives the double fault handler a TSS (IST) stack
    GDT::init();
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

    // Disks: ATA PIO polls, it needs neither interrupts nor the scheduler
    Fs::Vfs& vfs = s_vfs.construct();
    Fs::mountAtaDisks(vfs, s_disks);

    // Interrupt controller: the 8259 PIC (there is no APIC driver yet). The ISR
    // dispatcher is the one place that needs it without being handed it.
    Drivers::PIC& pic = s_pic.construct();
    Handlers::setInterruptController(&pic);

    // Scheduler tick: the PIT, delivered through the PIC
    Drivers::PIT& pit = s_pit.construct(pic);

    IOPort::enableInterrupts();

    // The boot flow becomes the scheduler's first task
    Threading::Scheduler& scheduler = s_scheduler.construct(pit);

    // The keyboard and COM1 arrive through one queue: the shell reads it,
    // and a program the shell runs (file descriptor 0)
    Drivers::CharacterQueue& input = s_input.construct();

    // User programs: loaded from the VFS, run on threads of this scheduler.
    // The int 0x80 and ring 3 fault handlers reach it through setRuntime
    Process::Runtime& runtime = s_runtime.construct(pmm, kernelSpace, vfs, input);
    Process::setRuntime(&runtime);

#ifdef ATLOS_SELFTEST
    int failed = SelfTest::run(pmm, kernelSpace, scheduler, vfs, runtime, input);
    Core::haltWithStatus(failed ? Core::ExitStatus::Failure : Core::ExitStatus::Success);
#endif

#ifdef ATLOS_CRASHTEST
    CrashTest::run();
#endif

    // Only now the devices feed the queue: the self tests push into it themselves
    s_keyboard.construct(pic, input);
    serial.startReceiving(pic, input);

    // Runs in the boot flow (task 0) for good; reading sleeps in hlt
    static Shell::Context s_context { pmm, kernelSpace, vfs, s_disks, s_pci, runtime, &vga,
        serial.isPresent() ? &serial : nullptr };
    s_shell.construct(input, s_context).run();
}
