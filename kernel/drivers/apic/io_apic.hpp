/**
 * @file io_apic.hpp
 * @author Oguzhan KAYNAK
 * @brief An I/O APIC: its interrupt inputs (pins) and the redirection table
 * that sends each one to a vector of a local APIC.
 */

#pragma once

#include <stdint.h>

namespace Drivers {

class IoApic {
public:
    /// Registers behind the index (IOREGSEL) and data (IOWIN) windows
    enum Register : uint8_t {
        RegisterId = 0x00, ///< bits 27..24
        RegisterVersion = 0x01, ///< bits 7..0 the version, 23..16 the last pin
        RegisterRedirection = 0x10, ///< pin n: 0x10 + 2n (low half), 0x11 + 2n (high half)
    };

    /// Bits of a redirection entry (64 bit)
    enum RedirectionBit : uint64_t {
        RedirectionActiveLow = 1 << 13, ///< polarity: 0 active high
        RedirectionLevel = 1 << 15, ///< trigger: 0 edge
        RedirectionMasked = 1 << 16,
        // Delivery mode 0 (fixed) and destination mode 0 (physical): the vector
        // in bits 7..0 goes to the local APIC whose id is in bits 63..56
    };
    static constexpr uint32_t destinationShift = 56;

    /// registers: the I/O APIC's physical address mapped uncached (AddressSpace::mapDevice)
    IoApic(volatile void* registers, uint64_t physAddress, uint32_t gsiBase);

    IoApic(const IoApic&) = delete;
    IoApic& operator=(const IoApic&) = delete;

    uint32_t read(uint8_t reg) const;
    void write(uint8_t reg, uint32_t value);

    uint8_t id() const { return (read(RegisterId) >> 24) & 0x0F; }
    uint8_t version() const { return read(RegisterVersion) & 0xFF; }
    uint32_t pinCount() const { return ((read(RegisterVersion) >> 16) & 0xFF) + 1; }
    uint32_t gsiBase() const { return m_gsiBase; }
    uint64_t physAddress() const { return m_physAddress; }

    /// This I/O APIC has the input of global system interrupt gsi
    bool handles(uint32_t gsi) const { return gsi >= m_gsiBase && gsi - m_gsiBase < pinCount(); }

    uint64_t redirection(uint32_t pin) const;
    void setRedirection(uint32_t pin, uint64_t entry);

    /// Mask every pin
    void maskAll();

private:
    volatile uint32_t* m_registers;
    uint64_t m_physAddress;
    uint32_t m_gsiBase;
};

} // namespace Drivers
