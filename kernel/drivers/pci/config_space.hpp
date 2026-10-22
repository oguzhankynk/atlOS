/**
 * @file config_space.hpp
 * @author Oguzhan KAYNAK
 * @brief PCI configuration space: 256 bytes per device function that say
 * what it is (vendor, device, class) and where its registers are (BARs).
 */

#pragma once

#include <stdint.h>

namespace Drivers {

/// Where a function sits: bus 0..255, device 0..31, function 0..7 ("00:1f.2")
struct PciAddress {
    uint8_t bus;
    uint8_t device;
    uint8_t function;
};

/// Reads the configuration space. A function that is not there reads as all ones.
class PciConfigSpace {
public:
    virtual ~PciConfigSpace() = default;

    /// The 32 bits at offset (a multiple of 4, below 256).
    virtual uint32_t read32(PciAddress address, uint8_t offset) = 0;

    uint16_t read16(PciAddress address, uint8_t offset)
    {
        return (uint16_t)(read32(address, offset & 0xFC) >> ((offset & 2) * 8));
    }

    uint8_t read8(PciAddress address, uint8_t offset)
    {
        return (uint8_t)(read32(address, offset & 0xFC) >> ((offset & 3) * 8));
    }
};

/**
 * @brief Configuration Mechanism #1: write the function and offset to port
 * 0xCF8, read the value from port 0xCFC. Every PC has it, PCIe ones too
 * (they show the first 256 bytes of each function this way).
 */
class PciPortConfigSpace final : public PciConfigSpace {
public:
    static constexpr uint16_t addressPort = 0xCF8;
    static constexpr uint16_t dataPort = 0xCFC;

    uint32_t read32(PciAddress address, uint8_t offset) override;
};

} // namespace Drivers
