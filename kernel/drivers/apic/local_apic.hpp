/**
 * @file local_apic.hpp
 * @author Oguzhan KAYNAK
 * @brief The local APIC of the processor (xAPIC: its registers in memory):
 * its id, enabling it, the end of an interrupt, and its timer's registers.
 */

#pragma once

#include <stdint.h>

namespace Drivers {

class LocalApic {
public:
    /// Register offsets from the base; every register is 32 bit, 16 byte aligned
    enum Register : uint32_t {
        RegisterId = 0x020, ///< bits 31..24: this APIC's id
        RegisterVersion = 0x030, ///< bits 7..0 the version, 23..16 the last LVT entry
        RegisterTaskPriority = 0x080, ///< interrupts at or below this priority class wait
        RegisterEndOfInterrupt = 0x0B0, ///< write 0: the interrupt in service is done
        RegisterSpurious = 0x0F0, ///< bit 8: the APIC is enabled; bits 7..0: the spurious vector
        RegisterInService = 0x100, ///< 8 registers, one bit per vector being handled
        RegisterErrorStatus = 0x280,
        RegisterLvtTimer = 0x320, ///< the local vector table: the timer, ...
        RegisterLvtLint0 = 0x350, ///< ... the LINT0 and LINT1 pins, ...
        RegisterLvtLint1 = 0x360,
        RegisterLvtError = 0x370, ///< ... and the APIC's own errors
        RegisterTimerInitialCount = 0x380, ///< writing it starts the timer
        RegisterTimerCurrentCount = 0x390, ///< counts down to 0
        RegisterTimerDivide = 0x3E0, ///< the bus clock divided by 1 .. 128
    };

    /// Bits of a local vector table entry
    enum LvtBit : uint32_t {
        LvtMasked = 1 << 16,
        LvtTimerPeriodic = 1 << 17, ///< the timer: reload the initial count at 0
    };

    /// IA32_APIC_BASE: bit 11 enables the APIC, bit 8 marks the bootstrap processor
    static constexpr uint32_t msrApicBase = 0x1B;

    /// The processor has a local APIC (CPUID.1:EDX bit 9)
    static bool isPresent();

    /// registers: physAddress mapped uncached (AddressSpace::mapDevice)
    LocalApic(volatile void* registers, uint64_t physAddress);

    LocalApic(const LocalApic&) = delete;
    LocalApic& operator=(const LocalApic&) = delete;

    /**
     * @brief Turn the APIC on (IA32_APIC_BASE, then the spurious register),
     * accept every priority, and send spurious interrupts to spuriousVector.
     */
    void enable(uint8_t spuriousVector);

    uint32_t read(Register reg) const { return m_registers[reg / 4]; }
    void write(Register reg, uint32_t value) { m_registers[reg / 4] = value; }

    void endOfInterrupt() { write(RegisterEndOfInterrupt, 0); }

    uint8_t id() const { return read(RegisterId) >> 24; }
    uint8_t version() const { return read(RegisterVersion) & 0xFF; }
    uint32_t lvtCount() const { return ((read(RegisterVersion) >> 16) & 0xFF) + 1; }
    uint64_t physAddress() const { return m_physAddress; }

private:
    volatile uint32_t* m_registers;
    uint64_t m_physAddress;
};

} // namespace Drivers
