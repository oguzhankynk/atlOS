/**
 * @file serial.hpp
 * @author Oguzhan KAYNAK
 * @brief  Serial port handling functions.
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
#ifndef D44D62C2_63FE_47DE_A6E8_802E23E4BABB
#define D44D62C2_63FE_47DE_A6E8_802E23E4BABB

#include <stdint.h>
/**
 * @namespace SerialPort
 * @brief Provides functions for serial port initialization, reading, and writing.
 */
namespace SerialPort
{

    /**
     * @brief Initializes the serial port.
     *
     * Sets up the serial port for communication. This function must be called before
     * any other serial port functions.
     *
     * @return 0 on success, -1 on failure.
     */
    int Init();

    /**
     * @brief Reads a byte from the serial port.
     *
     * @return The byte of data read from the serial port.
     */
    char read_serial();

    /**
     * @brief Writes a byte to the serial port.
     *
     * @param a The byte of data to send.
     */
    void write_serial(char a);

    /**
     * @brief Writes bytes to the serial port.
     *
     * @param data The bytes of data to send.
     * @param size The byte number of data
     */
    void write_serial(const char *data, size_t size);

} // namespace SerialPort

#endif /* D44D62C2_63FE_47DE_A6E8_802E23E4BABB */
