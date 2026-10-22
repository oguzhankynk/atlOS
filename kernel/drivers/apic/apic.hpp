/**
 * @file apic.hpp
 * @author Oguzhan KAYNAK
 * @brief The APIC as the system's interrupt controller: the ISA IRQs arrive
 * through the I/O APICs, routed as the MADT says, and the local APIC ends them.
 */

#pragma once

#include <stdint.h>

#include <drivers/interrupt_controller.hpp>
#include <firmware/acpi/madt.hpp>

namespace Drivers {

class IoApic;
class LocalApic;

class ApicInterruptController final : public InterruptController {
public:
    /// ISA IRQ n arrives on vector irqVectorBase + n, as with the PIC
    static constexpr uint8_t irqVectorBase = 0x20;
    static constexpr uint8_t irqCount = 16;

    /// The local APIC's own interrupts
    static constexpr uint8_t timerVector = 0x40;
    static constexpr uint8_t spuriousVector = 0xFF; ///< no end of interrupt for it

    static constexpr uint32_t maxIoApics = Firmware::MadtInfo::maxIoApics;

    /**
     * @brief Enable the local APIC, mask every pin of the I/O APICs, then
     * route the ISA IRQs (masked) to their vectors on this processor.
     * local, the I/O APICs and madt must outlive the controller.
     */
    ApicInterruptController(LocalApic& local, IoApic* const* ioApics, uint32_t ioApicCount, const Firmware::MadtInfo& madt);

    ApicInterruptController(const ApicInterruptController&) = delete;
    ApicInterruptController& operator=(const ApicInterruptController&) = delete;

    bool attach(Irq irq, InterruptHandlerFunction handler, void* arg) override;
    void mask(Irq irq) override;
    void unmask(Irq irq) override;
    void endOfInterrupt(uint32_t vector) override;

    /// The I/O APIC pin irq is routed to. false if it is not routed (no I/O APIC has its GSI).
    bool locate(Irq irq, const IoApic*& ioApic, uint32_t& pin) const;

    LocalApic& localApic() const { return m_local; }
    uint32_t ioApicCount() const { return m_ioApicCount; }
    const IoApic& ioApic(uint32_t index) const { return *m_ioApics[index]; }
    const Firmware::MadtInfo& madt() const { return m_madt; }

private:
    struct Route {
        IoApic* ioApic; ///< nullptr: not routed
        uint32_t pin;
    };

    void route(uint8_t irq);

    LocalApic& m_local;
    IoApic* m_ioApics[maxIoApics] {};
    uint32_t m_ioApicCount {};
    const Firmware::MadtInfo& m_madt;
    Route m_routes[irqCount] {};
};

} // namespace Drivers
