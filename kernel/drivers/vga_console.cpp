/**
 * @file vga_console.cpp
 * @author Oguzhan KAYNAK
 * @brief VGA text mode console (80x25)
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
 *
 */

#include <drivers/vga.hpp>
#include <drivers/vga_console.hpp>

namespace Drivers {

VGATextConsole::VGATextConsole(volatile uint16_t* buffer)
    : m_buffer(buffer)
    , m_color(vgaEntryColor(VgaColorLightGrey, VgaColorBlack))
{
    for (size_t y = 0; y < height; y++) {
        for (size_t x = 0; x < width; x++)
            putEntryAt(' ', m_color, x, y);
    }
}

void VGATextConsole::write(const char* data, size_t size)
{
    for (size_t i = 0; i < size; i++)
        putChar(data[i]);
}

void VGATextConsole::putEntryAt(unsigned char c, uint8_t color, size_t x, size_t y)
{
    m_buffer[y * width + x] = vgaEntry(c, color);
}

// Move to the start of the next line, scrolling at the bottom
void VGATextConsole::newLine()
{
    m_column = 0;
    if (m_row + 1 == height)
        scroll();
    else
        m_row++;
}

void VGATextConsole::putChar(char c)
{
    if (c == '\n') {
        newLine();
        return;
    }

    if (c == '\t') {
        for (int i = 0; i < 4; i++)
            putChar(' ');
        return;
    }

    putEntryAt(c, m_color, m_column, m_row);
    if (++m_column == width)
        newLine();
}

// Move every row up by one and clear the last one
void VGATextConsole::scroll()
{
    for (size_t row = 0; row < height - 1; row++) {
        for (size_t col = 0; col < width; col++)
            m_buffer[row * width + col] = m_buffer[(row + 1) * width + col];
    }

    for (size_t col = 0; col < width; col++)
        putEntryAt(' ', m_color, col, height - 1);
}

} // namespace Drivers
