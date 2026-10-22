/**
 * @file config_space.cpp
 * @author Oguzhan KAYNAK
 * @brief Drivers::PciPortConfigSpace: the configuration space through ports 0xCF8 and 0xCFC.
 */

#include <drivers/pci/config_space.hpp>

#include <arch/x86_64/io/ioport.hpp>
#include <sync/interrupt_guard.hpp>

namespace Drivers {

uint32_t PciPortConfigSpace::read32(PciAddress address, uint8_t offset)
{
    // bit 31 enable, 23..16 bus, 15..11 device, 10..8 function, 7..2 the register
    uint32_t value = 0x80000000U | ((uint32_t)address.bus << 16) | ((uint32_t)(address.device & 0x1F) << 11)
        | ((uint32_t)(address.function & 0x07) << 8) | (offset & 0xFC);

    // Two port accesses: an interrupt handler between them must not move 0xCF8
    Sync::InterruptGuard guard;
    IOPort::outportl(addressPort, value);
    return IOPort::inportl(dataPort);
}

} // namespace Drivers
