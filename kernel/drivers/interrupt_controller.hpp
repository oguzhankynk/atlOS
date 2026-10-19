/**
 * @file interrupt_controller.hpp
 * @brief Interface of the hardware that delivers IRQs (8259 PIC today, APIC later).
 */

#pragma once

#include <arch/x86_64/idt/handlers.hpp>
#include <stdint.h>

namespace Drivers {

/**
 * @brief Legacy ISA IRQ lines. An APIC routes the same numbers through the I/O APIC.
 */
enum class Irq : uint8_t {
    Timer = 0, // PIT
    Keyboard = 1,
    Cascade = 2, // 8259: slave PIC on the master
    COM2 = 3,
    COM1 = 4,
    LPT2 = 5,
    Floppy = 6,
    LPT1 = 7,
    RTC = 8,
    Redirected = 9,
    Free10 = 10,
    Free11 = 11,
    PS2Mouse = 12,
    FPU = 13,
    PrimaryATA = 14,
    SecondaryATA = 15
};

class InterruptController {
public:
    /**
     * @brief Call handler(arg) when irq fires. The line stays masked until unmask().
     * @return false if the IRQ can not be routed.
     */
    virtual bool attach(Irq irq, InterruptHandlerFunction handler, void* arg) = 0;

    virtual void mask(Irq irq) = 0;
    virtual void unmask(Irq irq) = 0;

    /**
     * @brief Acknowledge an interrupt. The interrupt dispatcher calls this for every
     * vector >= 32 before running the handler; vectors that are not IRQs of this
     * controller are ignored.
     */
    virtual void endOfInterrupt(uint32_t vector) = 0;

protected:
    // Never deleted through the interface
    ~InterruptController() = default;
};

} // namespace Drivers
