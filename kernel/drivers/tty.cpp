/**
 * @file tty.cpp
 * @author Oguzhan KAYNAK
 * @brief Source file implementing functions to terminal devices (TTY).
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

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <drivers/tty.hpp>
#include <drivers/vga.hpp>

static const size_t s_vgaWidth = 80;
static const size_t s_vgaHeight = 25;
static volatile uint16_t* s_vgaMemory = (volatile uint16_t*)0xB8000; // Paging disable
// static volatile uint16_t *const VGA_MEMORY = (volatile uint16_t *)0xC03FF000; // Paging enable

static size_t s_terminalRow;
static size_t s_terminalColumn;
static uint8_t s_terminalColor;
static volatile uint16_t* s_terminalBuffer;

static void terminalScroll(void);

void terminalInitialize(void)
{
    s_terminalRow = 0;
    s_terminalColumn = 0;
    s_terminalColor = vgaEntryColor(VgaColorLightGrey, VgaColorBlack);
    s_terminalBuffer = s_vgaMemory;
    for (size_t y = 0; y < s_vgaHeight; y++) {
        for (size_t x = 0; x < s_vgaWidth; x++) {
            const size_t index = y * s_vgaWidth + x;
            s_terminalBuffer[index] = vgaEntry(' ', s_terminalColor);
        }
    }
}

void terminalSetcolor(uint8_t color)
{
    s_terminalColor = color;
}

void terminalPutentryat(unsigned char c, uint8_t color, size_t x, size_t y)
{
    const size_t index = y * s_vgaWidth + x;
    s_terminalBuffer[index] = vgaEntry(c, color);
}

void terminalPutchar(char c)
{
    if (c == '\n') {
        if (s_terminalRow + 1 == s_vgaHeight) {
            terminalScroll();
            // If we have reached the last line, we won't be able to go to
            // another line, so we reset the column to the first position.
            s_terminalColumn = 0;
        } else {
            // Move the cursor to the beginning of the next line.
            s_terminalRow++;
            s_terminalColumn = 0;
        }
    } else if (c == '\t') {
        terminalPutchar(' ');
        terminalPutchar(' ');
        terminalPutchar(' ');
        terminalPutchar(' ');
    } else {
        // Put the character at the current cursor position
        terminalPutentryat(c, s_terminalColor, s_terminalColumn, s_terminalRow);

        // If the cursor reaches the end of the line, move it to the
        // beginning of the next line. If we have reached the last line,
        // scroll the terminal.
        if (++s_terminalColumn == s_vgaWidth) {
            if (s_terminalRow + 1 == s_vgaHeight) {
                terminalScroll();
                // If we have reached the last line, we won't be able to go to
                // another line, so we reset the column to the first position.
                s_terminalColumn = 0;
            } else {
                // Move the cursor to the beginning of the next line.
                s_terminalRow++;
                s_terminalColumn = 0;
            }
        }
    }
}

void terminalWrite(const char* data, size_t size)
{
    for (size_t i = 0; i < size; i++)
        terminalPutchar(data[i]);
}

void terminalWritestring(const char* data)
{
    terminalWrite(data, strlen(data));
}

/// @brief Scrolls the terminal buffer up by one row.
static void terminalScroll(void)
{
    // Scroll the buffer up by one row.
    for (unsigned int row = 0; row < s_vgaHeight - 1; row++) {
        for (unsigned int col = 0; col < s_vgaWidth; col++) {
            uint16_t entry = s_terminalBuffer[(row + 1) * s_vgaWidth + col];
            char c = entry & 0x00FF;
            uint8_t color = entry >> 8;

            if (c == '\0')
                continue;

            terminalPutentryat(c, color, col, row);
        }
    }

    // Clear the last row.
    for (unsigned int i = 0; i < s_vgaWidth; i++) {
        terminalPutentryat(' ', s_terminalColor, i, s_vgaHeight - 1);
    }
}
