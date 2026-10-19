/**
 * @file selftest.hpp
 * @author Oguzhan KAYNAK
 * @brief In-kernel self tests for the memory managers, heap and scheduler.
 *
 * Built with -DATLOS_SELFTEST=ON (see test.sh), kernelMain runs the tests
 * instead of the demo threads and reports the result to QEMU.
 */

#pragma once

namespace SelfTest {
/**
 * @brief Run all tests. Needs Memory::init and Scheduler::init to be done.
 *
 * @return Number of failed checks.
 */
int run();

/**
 * @brief Exit QEMU through the isa-debug-exit device (iobase 0xf4).
 * QEMU exits with (code << 1) | 1. Does nothing without the device.
 */
void exitQemu(int code);

} // namespace SelfTest
