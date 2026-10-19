/**
 * @file selftest.hpp
 * @author Oguzhan KAYNAK
 * @brief In-kernel self tests for the memory managers, heap and scheduler.
 *
 * Built with -DATLOS_SELFTEST=ON (scripts/build.sh selftest), kernelMain runs the tests
 * instead of the demo threads and reports the result to QEMU.
 */

#pragma once

namespace Memory {
class PhysicalMemoryManager;
}

namespace SelfTest {
/**
 * @brief Run all tests. Needs the memory managers and Scheduler::init to be set up.
 *
 * @param pmm The kernel's physical memory manager, used for leak checks around
 *            the VMM, heap and thread tests.
 *
 * @return Number of failed checks.
 */
int run(Memory::PhysicalMemoryManager& pmm);

/**
 * @brief Exit QEMU through the isa-debug-exit device (iobase 0xf4).
 * QEMU exits with (code << 1) | 1. Does nothing without the device.
 */
void exitQemu(int code);

} // namespace SelfTest
