/**
 * @file pci.hpp
 * @author Oguzhan KAYNAK
 * @brief Finding the PCI (and PCIe) functions of the machine and naming
 * what they are: the scan, the class, vendor and capability names.
 */

#pragma once

#include <stdint.h>

#include <drivers/pci/config_space.hpp>

namespace Drivers {

/// Offsets of the configuration header every function has
enum PciRegister : uint8_t {
    PciVendorId = 0x00, ///< 16 bit; 0xFFFF: no function here
    PciDeviceId = 0x02, ///< 16 bit, chosen by the vendor
    PciCommand = 0x04, ///< 16 bit: what the function may do (I/O, memory, bus master)
    PciStatus = 0x06, ///< 16 bit; bit 4: it has a capability list
    PciRevision = 0x08,
    PciProgIf = 0x09, ///< the register interface, within the subclass
    PciSubclass = 0x0A,
    PciClass = 0x0B,
    PciHeaderType = 0x0E, ///< bits 0..6: the layout (0 device, 1 bridge); bit 7: more functions
    PciBar0 = 0x10, ///< the base address registers: 6 of a device, 2 of a bridge
    PciSecondaryBus = 0x19, ///< a bridge: the bus behind it
    PciCapabilities = 0x34, ///< the offset of the first capability
    PciInterruptLine = 0x3C, ///< the IRQ the firmware routed it to
    PciInterruptPin = 0x3D, ///< 0 none, 1..4 INTA..INTD
};

/// Command register bits
enum PciCommandBit : uint16_t {
    PciCommandIo = 1 << 0, ///< answers its I/O BARs
    PciCommandMemory = 1 << 1, ///< answers its memory BARs
    PciCommandBusMaster = 1 << 2, ///< may do DMA
    PciCommandNoInterrupts = 1 << 10, ///< INTx disabled
};

static constexpr uint16_t pciStatusCapabilities = 1 << 4;
static constexpr uint16_t pciNoVendor = 0xFFFF;
static constexpr uint8_t pciMultiFunction = 0x80;
static constexpr uint8_t pciCapabilityExpress = 0x10;

/// One function the scan found, with the fields that say what it is
struct PciDevice {
    uint16_t vendorId;
    uint16_t deviceId;
    PciAddress address;
    uint8_t classCode;
    uint8_t subclass;
    uint8_t progIf;
    uint8_t revision;
    uint8_t headerType; ///< without the multi function bit
};

/**
 * @brief Try every bus, device and function, store what answers into devices.
 * A device whose function 0 has no multi function bit has only function 0.
 * @return How many were stored (at most capacity).
 */
uint32_t scanPci(PciConfigSpace& config, PciDevice* devices, uint32_t capacity);

/// What the class and subclass mean, e.g. "IDE controller"; never nullptr.
const char* pciClassName(uint8_t classCode, uint8_t subclass);

/// A few well known vendors, e.g. "Intel"; nullptr for the rest.
const char* pciVendorName(uint16_t vendorId);

/// A capability's name, e.g. "PCI Express"; nullptr for one without a name here.
const char* pciCapabilityName(uint8_t id);

} // namespace Drivers
