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

#include <kernel/tty.h>
#include <kernel/vga.h>

static const size_t VGA_WIDTH = 80;
static const size_t VGA_HEIGHT = 25;
//static volatile uint16_t* const VGA_MEMORY = (volatile uint16_t*) 0xB8000; //Paging disable
static volatile uint16_t* const VGA_MEMORY = (volatile uint16_t*) 0xC00B8000; //Paging enable

static size_t terminal_row;
static size_t terminal_column;
static uint8_t terminal_color;
static volatile uint16_t* terminal_buffer;

static void terminal_scroll(void);

void terminal_initialize(void) 
{
	terminal_row = 0;
	terminal_column = 0;
	terminal_color = vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
	terminal_buffer = VGA_MEMORY;
	for (size_t y = 0; y < VGA_HEIGHT; y++) {
		for (size_t x = 0; x < VGA_WIDTH; x++) {
			const size_t index = y * VGA_WIDTH + x;
			terminal_buffer[index] = vga_entry(' ', terminal_color);
		}
	}
}

void terminal_setcolor(uint8_t color) 
{
	terminal_color = color;
}

void terminal_putentryat(unsigned char c, uint8_t color, size_t x, size_t y) 
{
	const size_t index = y * VGA_WIDTH + x;
	terminal_buffer[index] = vga_entry(c, color);
}

void terminal_putchar(char c) 
{
    if (c == '\n') 
	{
        if (terminal_row + 1 == VGA_HEIGHT) 
		{
            terminal_scroll();
            // If we have reached the last line, we won't be able to go to
            // another line, so we reset the column to the first position.
            terminal_column = 0;
        } 
		else 
		{
            // Move the cursor to the beginning of the next line.
            terminal_row++;
            terminal_column = 0;
        }
    }
	else if (c=='\t')
	{
		terminal_putchar(' ');
		terminal_putchar(' ');
		terminal_putchar(' ');
		terminal_putchar(' ');
	}
	else 
	{
        // Put the character at the current cursor position 
        terminal_putentryat(c, terminal_color, terminal_column, terminal_row);

        // If the cursor reaches the end of the line, move it to the
        // beginning of the next line. If we have reached the last line,
        // scroll the terminal.
        if (++terminal_column == VGA_WIDTH) 
		{
            terminal_column = 0;
            if (++terminal_row == VGA_HEIGHT) 
			{
                terminal_scroll();
            }
        }
    }
}

void terminal_write(const char* data, size_t size) 
{
	for (size_t i = 0; i < size; i++)
		terminal_putchar(data[i]);
}

void terminal_writestring(const char* data) 
{
	terminal_write(data, strlen(data));
}

/// @brief Scrolls the terminal buffer up by one row.
static void terminal_scroll(void)
{
    // Scroll the buffer up by one row.
	for(unsigned int row = 0; row<VGA_HEIGHT-1; row++)
	{
		for(unsigned int col=0; col< VGA_WIDTH; col++)
		{
			uint16_t entry = terminal_buffer[(row+1)*VGA_WIDTH + col];
			char c = entry & 0x00FF;
			uint8_t color = entry >> 8;

			if(c=='\0')
				continue;

			terminal_putentryat(c, color, col, row);
		}
	}


    // Clear the last row.
    for (unsigned int i = 0; i < VGA_WIDTH; i++) 
	{
        terminal_putentryat(' ', terminal_color, i, VGA_HEIGHT - 1);
    }
}
