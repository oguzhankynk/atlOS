/**
 * @file pic.hpp
 * @author Oguzhan KAYNAK ()
 * @brief Class for interfacing with the 8259 Programmable Interrupt Controller (PIC).
 *
 * Copyright © 2024 Oguzhan KAYNAK
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the “Software”), to deal in the Software
 * without  restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit
 * persons to whom the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
 * PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#pragma once

#include <drivers/interrupt_controller.hpp>
#include <stdint.h>

namespace Drivers {

/**
 * @brief The 8259 PIC pair (master and slave) as the system's interrupt controller.
 */
class PIC final : public InterruptController {
public:
    /// IRQ 0..15 are delivered on vectors vectorBase .. vectorBase + 15
    static constexpr uint8_t vectorBase = 0x20;

    /**
     * @brief Remap the IRQs to vectorBase and mask every line. Interrupts are
     * not enabled here (the caller does sti when the system is ready).
     */
    PIC();

    bool attach(Irq irq, InterruptHandlerFunction handler, void* arg) override;
    void mask(Irq irq) override;
    void unmask(Irq irq) override;
    void endOfInterrupt(uint32_t vector) override;
};

} // namespace Drivers
