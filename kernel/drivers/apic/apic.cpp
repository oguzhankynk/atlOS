/**
 * @file apic.cpp
 * @author Oguzhan KAYNAK
 * @brief Drivers::ApicInterruptController: routing the ISA IRQs through the I/O APICs.
 */

#include <drivers/apic/apic.hpp>

#include <stdio.h>

#include <drivers/apic/io_apic.hpp>
#include <drivers/apic/local_apic.hpp>

namespace Drivers {

// A spurious interrupt is not in service: nothing to end, nothing to do
static void ignoreSpurious(void*)
{
}

ApicInterruptController::ApicInterruptController(LocalApic& local, IoApic* const* ioApics, uint32_t ioApicCount,
    const Firmware::MadtInfo& madt)
    : m_local(local)
    , m_madt(madt)
{
    for (uint32_t i = 0; i < ioApicCount && i < maxIoApics; i++) {
        m_ioApics[m_ioApicCount++] = ioApics[i];
        ioApics[i]->maskAll();
    }

    Handlers::addInterruptHandler(spuriousVector, ignoreSpurious, nullptr);
    m_local.enable(spuriousVector);

    for (uint8_t irq = 0; irq < irqCount; irq++)
        route(irq);
}

void ApicInterruptController::route(uint8_t irq)
{
    Firmware::IsaRoute isa = Firmware::isaRoute(m_madt, irq);

    // A GSI another IRQ is overridden to belongs to that one (IRQ 0 on GSI 2 takes it from IRQ 2)
    for (uint32_t i = 0; i < m_madt.overrideCount; i++) {
        if (m_madt.overrides[i].irq != irq && m_madt.overrides[i].gsi == isa.gsi)
            return;
    }

    for (uint32_t i = 0; i < m_ioApicCount; i++) {
        IoApic& ioApic = *m_ioApics[i];
        if (!ioApic.handles(isa.gsi))
            continue;

        // To this processor's local APIC, fixed delivery, masked until a driver unmasks it
        uint64_t entry = (irqVectorBase + irq) | IoApic::RedirectionMasked
            | ((uint64_t)m_local.id() << IoApic::destinationShift);
        if (isa.activeLow)
            entry |= IoApic::RedirectionActiveLow;
        if (isa.levelTriggered)
            entry |= IoApic::RedirectionLevel;

        uint32_t pin = isa.gsi - ioApic.gsiBase();
        ioApic.setRedirection(pin, entry);
        m_routes[irq] = { &ioApic, pin };
        return;
    }
    printf("APIC: IRQ %u (GSI %u) is on no I/O APIC\n", irq, isa.gsi);
}

bool ApicInterruptController::attach(Irq irq, InterruptHandlerFunction handler, void* arg)
{
    auto line = static_cast<uint8_t>(irq);
    if (line >= irqCount || !m_routes[line].ioApic)
        return false;

    return Handlers::addInterruptHandler(irqVectorBase + line, handler, arg);
}

void ApicInterruptController::mask(Irq irq)
{
    auto line = static_cast<uint8_t>(irq);
    if (line >= irqCount || !m_routes[line].ioApic)
        return;

    const Route& r = m_routes[line];
    r.ioApic->setRedirection(r.pin, r.ioApic->redirection(r.pin) | IoApic::RedirectionMasked);
}

void ApicInterruptController::unmask(Irq irq)
{
    auto line = static_cast<uint8_t>(irq);
    if (line >= irqCount || !m_routes[line].ioApic)
        return;

    const Route& r = m_routes[line];
    r.ioApic->setRedirection(r.pin, r.ioApic->redirection(r.pin) & ~(uint64_t)IoApic::RedirectionMasked);
}

void ApicInterruptController::endOfInterrupt(uint32_t vector)
{
    // The local APIC ends the interrupt in service; a level triggered I/O APIC
    // pin hears of it too (the EOI is broadcast)
    if ((vector >= irqVectorBase && vector < irqVectorBase + irqCount) || vector == timerVector)
        m_local.endOfInterrupt();
}

bool ApicInterruptController::locate(Irq irq, const IoApic*& ioApic, uint32_t& pin) const
{
    auto line = static_cast<uint8_t>(irq);
    if (line >= irqCount || !m_routes[line].ioApic)
        return false;

    ioApic = m_routes[line].ioApic;
    pin = m_routes[line].pin;
    return true;
}

} // namespace Drivers
