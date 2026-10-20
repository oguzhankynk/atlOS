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
    clear();
}

void VGATextConsole::clear()
{
    for (size_t y = 0; y < height; y++) {
        for (size_t x = 0; x < width; x++)
            putEntryAt(' ', m_color, x, y);
    }
    m_row = 0;
    m_column = 0;
}

// ANSI color numbers (black, red, green, yellow, blue, magenta, cyan, white)
// in the VGA palette order
static constexpr uint8_t ansiToVga[8] = { VgaColorBlack, VgaColorRed, VgaColorGreen, VgaColorBrown, VgaColorBlue,
    VgaColorMagenta, VgaColorCyan, VgaColorLightGrey };

void VGATextConsole::applyAnsi()
{
    uint8_t background = m_color & 0xF0;
    uint8_t foreground = m_color & 0x0F;
    if (!m_parameterCount)
        m_parameters[m_parameterCount++] = 0; // "ESC [ m" means reset

    for (uint32_t i = 0; i < m_parameterCount; i++) {
        uint32_t p = m_parameters[i];
        if (p == 0) {
            foreground = VgaColorLightGrey;
            m_bright = false;
        } else if (p == 1) {
            m_bright = true;
            foreground |= 0x08;
        } else if (p == 22) {
            m_bright = false;
            foreground &= 0x07;
        } else if (p >= 30 && p <= 37) {
            foreground = ansiToVga[p - 30] | (m_bright ? 0x08 : 0);
        } else if (p >= 90 && p <= 97) {
            foreground = ansiToVga[p - 90] | 0x08;
        } else if (p == 39) {
            foreground = VgaColorLightGrey | (m_bright ? 0x08 : 0);
        }
    }
    m_color = background | foreground;
}

// Unicode block elements -> their place in the VGA font (code page 437)
static unsigned char toCodePage437(uint32_t codePoint)
{
    switch (codePoint) {
    case 0x2580:
        return 0xDF; // ▀ upper half
    case 0x2584:
        return 0xDC; // ▄ lower half
    case 0x2588:
        return 0xDB; // █ full
    case 0x2591:
        return 0xB0; // ░
    case 0x2592:
        return 0xB1; // ▒
    case 0x2593:
        return 0xB2; // ▓
    default:
        return '?';
    }
}

void VGATextConsole::write(const char* data, size_t size)
{
    for (size_t i = 0; i < size; i++) {
        char c = data[i];
        auto byte = static_cast<unsigned char>(c);

        // UTF-8: 110xxxxx / 1110xxxx / 11110xxx start a sequence of 10xxxxxx bytes
        if (m_utf8Left) {
            if ((byte & 0xC0) == 0x80) {
                m_codePoint = (m_codePoint << 6) | (byte & 0x3F);
                if (--m_utf8Left == 0)
                    putEntryChar(toCodePage437(m_codePoint));
                continue;
            }
            m_utf8Left = 0; // broken sequence: start over with this byte
            putEntryChar('?');
        }
        if (byte >= 0xC0 && m_escape == Escape::None) {
            m_utf8Left = byte >= 0xF0 ? 3 : (byte >= 0xE0 ? 2 : 1);
            m_codePoint = byte & (0x3F >> m_utf8Left);
            continue;
        }

        // ESC [ params final: colors ("m") are applied, the rest skipped
        if (m_escape == Escape::Started) {
            m_escape = c == '[' ? Escape::Parameters : Escape::None;
            m_parameterCount = 0;
            continue;
        }
        if (m_escape == Escape::Parameters) {
            if (c >= '0' && c <= '9') {
                if (!m_parameterCount)
                    m_parameters[m_parameterCount++] = 0;
                uint32_t& p = m_parameters[m_parameterCount - 1];
                p = p * 10 + (c - '0');
            } else if (c == ';') {
                if (!m_parameterCount)
                    m_parameters[m_parameterCount++] = 0;
                if (m_parameterCount < 4)
                    m_parameters[m_parameterCount++] = 0;
            } else if (c >= 0x40 && c <= 0x7E) {
                if (c == 'm')
                    applyAnsi();
                m_escape = Escape::None;
            }
            continue;
        }
        if (c == '\x1b') {
            m_escape = Escape::Started;
            continue;
        }
        putChar(c);
    }
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

// A character cell as it is, with no meaning for \n, \t, \b
void VGATextConsole::putEntryChar(unsigned char c)
{
    putEntryAt(c, m_color, m_column, m_row);
    if (++m_column == width)
        newLine();
}

void VGATextConsole::putChar(char c)
{
    if (c == '\n') {
        newLine();
        return;
    }

    // Back one cell (to the end of the previous line at column 0); the cell
    // is not cleared, "\b \b" erases like a terminal
    if (c == '\b') {
        if (m_column) {
            m_column--;
        } else if (m_row) {
            m_row--;
            m_column = width - 1;
        }
        return;
    }

    if (c == '\t') {
        for (int i = 0; i < 4; i++)
            putChar(' ');
        return;
    }

    putEntryChar(static_cast<unsigned char>(c));
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
