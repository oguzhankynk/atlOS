/**
 * @file IOPort.cpp
 * @author Oguzhan KAYNAK ()
 * @brief Functions for performing input and output operations on I/O ports.
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
#include <kernel/sys/IOPort.hpp>
namespace IOPort
{

    /**
     * @brief Writes a byte to the specified I/O port.
     *
     * @param port The I/O port to write to.
     * @param value The byte value to write to the port.
     */
    void outportb(uint16_t port, uint8_t value)
    {
        asm volatile("outb %0, %1" ::"a"(value), "Nd"(port));
    }

    /**
     * @brief Reads a byte from the specified I/O port.
     *
     * @param port The I/O port to read from.
     * @return The byte value read from the port.
     */
    uint8_t inportb(uint16_t port)
    {
        uint8_t value;
        asm volatile("inb %1, %0" : "=a"(value) : "Nd"(port));
        return value;
    }

    /**
     * @brief Writes a word to the specified I/O port.
     *
     * @param port The I/O port to write to.
     * @param data The word value to write to the port.
     */
    void outportw(uint16_t port, uint16_t data)
    {
        asm volatile("outw %0, %1" : : "a"(data), "d"(port));
    }

    /**
     * @brief Reads a word from the specified I/O port.
     *
     * @param port The I/O port to read from.
     * @return uint16_t The word value read from the port.
     */
    uint16_t inportw(uint16_t port)
    {
        uint16_t result;

        asm volatile("inw %1, %0" : "=a"(result) : "d"(port));

        return result;
    }

    /**
     * @brief Enable Interrupts
     *
     */
    void EnableInterrupts(void)
    {
        asm volatile("sti");
    }

    /**
     * @brief Disable Interrupts
     *
     */
    void DisableInterrupts(void)
    {
        asm volatile("cli");
    }

} // namespace IOPort