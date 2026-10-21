/**
 * @file ioport.hpp
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
#pragma once

#include <stdint.h>

namespace IOPort {

/**
 * @brief Writes a byte to the specified I/O port.
 *
 * @param port The I/O port to write to.
 * @param value The byte value to write to the port.
 */
void outportb(uint16_t port, uint8_t value);

/**
 * @brief Reads a byte from the specified I/O port.
 *
 * @param port The I/O port to read from.
 * @return The byte value read from the port.
 */
uint8_t inportb(uint16_t port);

/**
 * @brief Writes a word to the specified I/O port.
 *
 * @param port The I/O port to write to.
 * @param data The word value to write to the port.
 */
void outportw(uint16_t port, uint16_t data);

/**
 * @brief Reads a word from the specified I/O port.
 *
 * @param port The I/O port to read from.
 * @return uint16_t The word value read from the port.
 */
uint16_t inportw(uint16_t port);

/// @brief Write a 32-bit value to an I/O port.
void outportl(uint16_t port, uint32_t data);

/// @brief Read a 32-bit value from an I/O port.
uint32_t inportl(uint16_t port);

/**
 * @brief Enable Interrupts
 *
 */
void enableInterrupts(void);

/**
 * @brief Disable Interrupts
 *
 */
void disableInterrupts(void);

/**
 * @brief Whether maskable interrupts are enabled (RFLAGS.IF).
 */
bool interruptsEnabled(void);

} // namespace IOPort
