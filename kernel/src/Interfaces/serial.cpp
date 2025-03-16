/**
 * @file serial.cpp
 * @author Oguzhan KAYNAK
 * @brief Implementation of serial port handling functions.
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
#include <Interfaces/serial.hpp>

#include <stdint.h>

#include <kernel/sys/IOPort.hpp>


namespace SerialPort
{

    static constexpr uint16_t PORT = 0x3F8;// COM1
    //static constexpr uint16_t PORT = 0x2F8;// COM2
    //static constexpr uint16_t PORT = 0x3E8;// COM3
    //static constexpr uint16_t PORT = 0x2E8;// COM4
    //static constexpr uint16_t PORT = 0x5F8;// COM5
    //static constexpr uint16_t PORT = 0x4F8;// COM6
    //static constexpr uint16_t PORT = 0x5E8;// COM7
    //static constexpr uint16_t PORT = 0x4E8;// COM8

    static int is_transmit_empty();

    /**
     * @brief Initializes the serial port.
     *
     * Sets up the serial port for communication. This function must be called before
     * any other serial port functions.
     *
     * @return 0 on success, -1 on failure.
     */
    int Init()
    {
        IOPort::outportb(PORT + 1, 0x00);    // Disable all interrupts
        IOPort::outportb(PORT + 3, 0x80);    // Enable DLAB (set baud rate divisor)
        IOPort::outportb(PORT + 0, 0x03);    // Set divisor to 3 (lo byte) 38400 baud
        IOPort::outportb(PORT + 1, 0x00);    //                  (hi byte)
        IOPort::outportb(PORT + 3, 0x03);    // 8 bits, no parity, one stop bit
        IOPort::outportb(PORT + 2, 0xC7);    // Enable FIFO, clear them, with 14-byte threshold
        IOPort::outportb(PORT + 4, 0x0B);    // IRQs enabled, RTS/DSR set
        IOPort::outportb(PORT + 4, 0x1E);    // Set in loopback mode, test the serial chip
        IOPort::outportb(PORT + 0, 0xAE);    // Test serial chip (send byte 0xAE and check if serial returns same byte)

        // Check if the serial port is properly initialized
        if (IOPort::inportb(PORT + 0) != 0xAE)
        {
            return -1; // Failure
        }

        IOPort::outportb(PORT + 4, 0x0F);
        return 0;
    }

    /**
     * @brief Reads a byte from the serial port.
     *
     * @return The byte of data read from the serial port.
     */
    char read_serial()
    {
        // Wait until the serial port is ready to read
        while (!(IOPort::inportb(PORT + 5) & 0x01));
        return IOPort::inportb(PORT);
    }

    /**
     * @brief Writes a byte to the serial port.
     *
     * @param a The byte of data to send.
     */
    void write_serial(char a)
    {
        // Wait until the serial port is ready to write
        while (!is_transmit_empty());
        IOPort::outportb(PORT, a);
    }

    void write_serial(const char* data, size_t size)
    {
        while (!is_transmit_empty());

        while (size > 0)
        {
            IOPort::outportb(PORT, *data);
            data++;
            size--;
        }
    }

    int is_transmit_empty() 
    {
        return IOPort::inportb(PORT + 5) & 0x20;
    }

} // namespace SerialPort