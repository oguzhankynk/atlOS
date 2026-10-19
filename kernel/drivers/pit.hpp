/**
 * @file pit.hpp
 * @author Oguzhan KAYNAK ()
 * @brief Class for interfacing with the (Intel 8253/8254) Programmable Interval Timer (PIT)
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
#include <drivers/tick_source.hpp>
#include <stdint.h>

namespace Drivers {

/**
 * @brief Channel 0 of the 8253/8254 PIT as a periodic tick, delivered on Irq::Timer.
 */
class PIT final : public TickSource {
public:
    explicit PIT(InterruptController& controller);

    /**
     * @brief Program channel 0 and attach handler to the timer IRQ.
     * Periods outside about 838 ns .. 54.9 ms do not fit the 16-bit divisor.
     */
    bool start(uint32_t periodNs, TickHandler handler, void* arg) override;

private:
    InterruptController& m_controller;
};

} // namespace Drivers
