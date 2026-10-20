/**
 * @file selftest.hpp
 * @author Oguzhan KAYNAK
 * @brief In-kernel self tests for the memory managers, heap and scheduler.
 *
 * Built with -DATLOS_SELFTEST=ON (scripts/build.sh selftest), kernelMain runs the tests
 * instead of the demo threads, then halts with the result
 * (Core::haltWithStatus).
 */

#pragma once

namespace Memory {
class AddressSpace;
class PhysicalMemoryManager;
}

namespace Threading {
class Scheduler;
}

namespace SelfTest {
/**
 * @brief Run all tests. Needs the memory managers and the scheduler to be set up.
 *
 * @param pmm The kernel's physical memory manager, used for leak checks around
 *            the VMM, heap and thread tests.
 * @param kernelSpace The kernel's address space (VMM tests, thread stacks).
 * @param scheduler The kernel's scheduler, which the calling flow is task 0 of.
 *
 * @return Number of failed checks.
 */
int run(Memory::PhysicalMemoryManager& pmm, Memory::AddressSpace& kernelSpace, Threading::Scheduler& scheduler);

} // namespace SelfTest
