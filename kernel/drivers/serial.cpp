/**
 * @file serial.cpp
 * @author Oguzhan KAYNAK
 * @brief 16550 UART serial port
 *
 * This file contains the implementations of the functions for initializing,
 * reading from, and writing to serial ports.
 *
 * For more information: https://wiki.osdev.org/Serial_Ports
 *
 * Copyright © 2024 Oguzhan KAYNAK
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the “Software”), to deal in the Software
 * without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit
 * persons to whom the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
 * PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include <drivers/serial.hpp>

#include <arch/x86_64/io/ioport.hpp>

namespace Drivers {

SerialPort::SerialPort(uint16_t port)
    : m_port(port)
{
    IOPort::outportb(m_port + 1, 0x00); // Disable all interrupts
    IOPort::outportb(m_port + 3, 0x80); // Enable DLAB (set baud rate divisor)
    IOPort::outportb(m_port + 0, 0x03); // Set divisor to 3 (lo byte) 38400 baud
    IOPort::outportb(m_port + 1, 0x00); //                  (hi byte)
    IOPort::outportb(m_port + 3, 0x03); // 8 bits, no parity, one stop bit
    IOPort::outportb(m_port + 2, 0xC7); // Enable FIFO, clear them, with 14-byte threshold
    IOPort::outportb(m_port + 4, 0x0B); // IRQs enabled, RTS/DSR set
    IOPort::outportb(m_port + 4, 0x1E); // Set in loopback mode, test the serial chip
    IOPort::outportb(m_port + 0, 0xAE); // Send 0xAE, it must come back

    if (IOPort::inportb(m_port + 0) != 0xAE)
        return;

    // Normal operation mode
    IOPort::outportb(m_port + 4, 0x0F);
    m_present = true;
}

bool SerialPort::isTransmitEmpty() const
{
    return IOPort::inportb(m_port + 5) & 0x20;
}

void SerialPort::writeByte(char c)
{
    // Wait for every byte: the FIFO only holds 16
    while (!isTransmitEmpty())
        ;
    IOPort::outportb(m_port, c);
}

void SerialPort::write(const char* data, size_t size)
{
    if (!m_present)
        return;

    for (size_t i = 0; i < size; i++) {
        if (data[i] == '\n')
            writeByte('\r');
        writeByte(data[i]);
    }
}

void SerialPort::startReceiving(InterruptController& controller, CharacterQueue& queue)
{
    if (!m_present)
        return;

    m_queue = &queue;
    controller.attach(m_port == com1 ? Irq::COM1 : Irq::COM2, onInterrupt, this);
    controller.unmask(m_port == com1 ? Irq::COM1 : Irq::COM2);

    // IER bit 0: interrupt when a byte has arrived (MCR OUT2, set above, routes it to the PIC)
    IOPort::outportb(m_port + 1, 0x01);
}

void SerialPort::onInterrupt(void* port)
{
    auto* self = static_cast<SerialPort*>(port);

    // LSR bit 0: a received byte is waiting. The FIFO may hold several
    while (IOPort::inportb(self->m_port + 5) & 0x01)
        self->m_queue->push(static_cast<char>(IOPort::inportb(self->m_port)));
}

} // namespace Drivers
