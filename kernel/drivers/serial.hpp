/**
 * @file serial.hpp
 * @author Oguzhan KAYNAK
 * @brief 16550 UART serial port
 *
 * This file contains the declarations for serial port handling, including
 * initialization, reading, and writing data to serial ports.
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

#pragma once

#include <drivers/character_sink.hpp>
#include <stddef.h>
#include <stdint.h>

namespace Drivers {

class SerialPort final : public CharacterSink {
public:
    static constexpr uint16_t com1 = 0x3F8;
    static constexpr uint16_t com2 = 0x2F8;

    /**
     * @brief Set up the UART at the given I/O port: 38400 baud, 8N1, FIFO on.
     * A loopback test decides whether the port is there, see isPresent().
     */
    explicit SerialPort(uint16_t port);

    /// False if the loopback test failed; writes are dropped then
    bool isPresent() const { return m_present; }

    void write(const char* data, size_t size) override;

    /// Wait for and return one received byte
    char read();

private:
    bool isTransmitEmpty() const;

    uint16_t m_port;
    bool m_present { false };
};

} // namespace Drivers
