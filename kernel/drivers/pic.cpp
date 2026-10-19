/**
 * @file pic.cpp
 * @author Oguzhan KAYNAK ()
 * @brief Implemantation for interfacing with the 8259 Programmable Interrupt Controller (PIC).
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

#include <arch/x86_64/idt/handlers.hpp>
#include <arch/x86_64/io/ioport.hpp>
#include <drivers/pic.hpp>

namespace Drivers {

// I/O port addresses for the master and slave PICs
static constexpr uint16_t masterCommandPort = 0x20;
static constexpr uint16_t masterDataPort = 0x21;
static constexpr uint16_t slaveCommandPort = 0xA0;
static constexpr uint16_t slaveDataPort = 0xA1;

static constexpr uint8_t endOfInterruptCommand = 0x20;
static constexpr uint8_t irqCount = 16;

PIC::PIC()
{
    // ICW1: start initialization, edge triggered, cascade mode, ICW4 follows
    IOPort::outportb(masterCommandPort, 0x11);
    IOPort::outportb(slaveCommandPort, 0x11);

    // ICW2: vector offsets
    IOPort::outportb(masterDataPort, vectorBase);
    IOPort::outportb(slaveDataPort, vectorBase + 8);

    // ICW3: slave on master IRQ2 / slave cascade identity 2
    IOPort::outportb(masterDataPort, 0x04);
    IOPort::outportb(slaveDataPort, 0x02);

    // ICW4: 8086 mode, normal EOI
    IOPort::outportb(masterDataPort, 0x01);
    IOPort::outportb(slaveDataPort, 0x01);

    // Every line masked until a driver unmasks it
    IOPort::outportb(masterDataPort, 0xFF);
    IOPort::outportb(slaveDataPort, 0xFF);
}

bool PIC::attach(Irq irq, InterruptHandlerFunction handler, void* arg)
{
    auto line = static_cast<uint8_t>(irq);
    if (line >= irqCount)
        return false;

    return Handlers::addInterruptHandler(vectorBase + line, handler, arg);
}

void PIC::mask(Irq irq)
{
    auto line = static_cast<uint8_t>(irq);
    uint16_t port = line < 8 ? masterDataPort : slaveDataPort;

    uint8_t bits = IOPort::inportb(port);
    IOPort::outportb(port, bits | (1 << (line % 8)));
}

void PIC::unmask(Irq irq)
{
    auto line = static_cast<uint8_t>(irq);
    uint16_t port = line < 8 ? masterDataPort : slaveDataPort;

    uint8_t bits = IOPort::inportb(port);
    IOPort::outportb(port, bits & ~(1 << (line % 8)));

    // Slave IRQs only reach the CPU through the cascade line on the master
    if (line >= 8)
        unmask(Irq::Cascade);
}

void PIC::endOfInterrupt(uint32_t vector)
{
    if (vector < vectorBase || vector >= vectorBase + irqCount)
        return;

    if (vector >= vectorBase + 8)
        IOPort::outportb(slaveCommandPort, endOfInterruptCommand);

    IOPort::outportb(masterCommandPort, endOfInterruptCommand);
}

} // namespace Drivers
