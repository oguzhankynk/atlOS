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

namespace Drivers {
class ApicInterruptController;
class ApicTimer;
class CharacterQueue;
class InterruptController;
}

namespace Firmware {
class AcpiTables;
class SmbiosTables;
}

namespace Fs {
class Vfs;
}

namespace Process {
class Runtime;
}

namespace SelfTest {

/// What kernelMain chose for the interrupts and the tick
struct Interrupts {
    const Firmware::AcpiTables& acpi; ///< loaded, or empty without ACPI
    Drivers::InterruptController& controller; ///< the APIC or the PIC
    Drivers::ApicInterruptController* apic; ///< nullptr: the PIC
    Drivers::ApicTimer* apicTimer; ///< nullptr: the PIT ticks
    unsigned tickVector; ///< where the scheduler's tick arrives
};

/**
 * @brief Run all tests. Needs the memory managers and the scheduler to be set up.
 *
 * @param pmm The kernel's physical memory manager, used for leak checks around
 *            the VMM, heap and thread tests.
 * @param kernelSpace The kernel's address space (VMM tests, thread stacks).
 * @param scheduler The kernel's scheduler, which the calling flow is task 0 of.
 * @param vfs The kernel's VFS; the disk tests use /disk0 if the test disk is attached.
 * @param runtime Runs the user programs from the test disk (/disk0/BIN).
 * @param input The runtime's keyboard queue: the tests type into it.
 * @param interrupts The interrupt controller and tick in use (the APIC tests).
 * @param smbios The machine's SMBIOS tables (empty if there were none).
 *
 * @return Number of failed checks.
 */
int run(Memory::PhysicalMemoryManager& pmm, Memory::AddressSpace& kernelSpace, Threading::Scheduler& scheduler, Fs::Vfs& vfs,
    Process::Runtime& runtime, Drivers::CharacterQueue& input, const Interrupts& interrupts,
    const Firmware::SmbiosTables& smbios);

} // namespace SelfTest
