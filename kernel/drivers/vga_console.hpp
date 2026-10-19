/**
 * @file vga_console.hpp
 * @author Oguzhan KAYNAK
 * @brief VGA text mode console (80x25)
 *  TTY represents a terminal device that interacts with the user.
 *  This header file defines functions that perform terminal input/output operations.
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

#include <drivers/character_sink.hpp>
#include <stddef.h>
#include <stdint.h>

namespace Drivers {

class VGATextConsole final : public CharacterSink {
public:
    static constexpr size_t width = 80;
    static constexpr size_t height = 25;

    /// Physical (and identity mapped) address of the VGA text buffer
    static constexpr uintptr_t textBufferAddress = 0xB8000;

    /**
     * @brief Clear buffer (width * height entries) and start at the top left.
     * The default is the real screen; tests pass memory of their own.
     */
    explicit VGATextConsole(volatile uint16_t* buffer = reinterpret_cast<volatile uint16_t*>(textBufferAddress));

    void write(const char* data, size_t size) override;

    void setColor(uint8_t color) { m_color = color; }

private:
    void putChar(char c);
    void putEntryAt(unsigned char c, uint8_t color, size_t x, size_t y);
    void newLine();
    void scroll();

    volatile uint16_t* m_buffer;
    size_t m_row {};
    size_t m_column {};
    uint8_t m_color;
};

} // namespace Drivers
