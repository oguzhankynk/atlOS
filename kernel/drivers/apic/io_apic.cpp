/**
 * @file io_apic.cpp
 * @author Oguzhan KAYNAK
 * @brief Drivers::IoApic: the index and data windows, the redirection table.
 */

#include <drivers/apic/io_apic.hpp>

#include <sync/interrupt_guard.hpp>

namespace Drivers {

// IOREGSEL at offset 0 selects a register, IOWIN at offset 0x10 reads or writes it
static constexpr uint32_t selectIndex = 0x00 / 4;
static constexpr uint32_t windowIndex = 0x10 / 4;

IoApic::IoApic(volatile void* registers, uint64_t physAddress, uint32_t gsiBase)
    : m_registers(static_cast<volatile uint32_t*>(registers))
    , m_physAddress(physAddress)
    , m_gsiBase(gsiBase)
{
}

uint32_t IoApic::read(uint8_t reg) const
{
    // Select, then access: an interrupt handler between the two must not move the selection
    Sync::InterruptGuard guard;
    m_registers[selectIndex] = reg;
    return m_registers[windowIndex];
}

void IoApic::write(uint8_t reg, uint32_t value)
{
    Sync::InterruptGuard guard;
    m_registers[selectIndex] = reg;
    m_registers[windowIndex] = value;
}

uint64_t IoApic::redirection(uint32_t pin) const
{
    uint8_t reg = RegisterRedirection + 2 * pin;
    return read(reg) | ((uint64_t)read(reg + 1) << 32);
}

void IoApic::setRedirection(uint32_t pin, uint64_t entry)
{
    // Masked while it changes, so a half written entry never delivers
    uint8_t reg = RegisterRedirection + 2 * pin;
    write(reg, RedirectionMasked);
    write(reg + 1, entry >> 32);
    write(reg, static_cast<uint32_t>(entry));
}

void IoApic::maskAll()
{
    for (uint32_t pin = 0; pin < pinCount(); pin++)
        setRedirection(pin, RedirectionMasked);
}

} // namespace Drivers
