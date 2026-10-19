/**
 * @file vga.h
 * @author Oguzhan KAYNAK
 * @brief Standard VGA chipset interaction
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
#ifndef ARCH_I386_VGA_H
#define ARCH_I386_VGA_H

#include <stdint.h>

#if defined(__cplusplus)
extern "C"
{
#endif

    /// @brief Enumerates VGA color codes.
    enum vga_color
    {
        VGA_COLOR_BLACK = 0,          ///< Black color (0).
        VGA_COLOR_BLUE = 1,           ///< Blue color (1).
        VGA_COLOR_GREEN = 2,          ///< Green color (2).
        VGA_COLOR_CYAN = 3,           ///< Cyan color (3).
        VGA_COLOR_RED = 4,            ///< Red color (4).
        VGA_COLOR_MAGENTA = 5,        ///< Magenta color (5).
        VGA_COLOR_BROWN = 6,          ///< Brown color (6).
        VGA_COLOR_LIGHT_GREY = 7,     ///< Light grey color (7).
        VGA_COLOR_DARK_GREY = 8,      ///< Dark grey color (8).
        VGA_COLOR_LIGHT_BLUE = 9,     ///< Light blue color (9).
        VGA_COLOR_LIGHT_GREEN = 10,   ///< Light green color (10).
        VGA_COLOR_LIGHT_CYAN = 11,    ///< Light cyan color (11).
        VGA_COLOR_LIGHT_RED = 12,     ///< Light red color (12).
        VGA_COLOR_LIGHT_MAGENTA = 13, ///< Light magenta color (13).
        VGA_COLOR_LIGHT_BROWN = 14,   ///< Light brown color (14).
        VGA_COLOR_WHITE = 15          ///< White color (15).
    };

    /// @brief Combines foreground and background colors to generate a VGA text mode color attribute.
    /// @param fg The foreground color.
    /// @param bg The background color.
    /// @return  The VGA text mode color attribute formed by combining the foreground and background colors.
    static inline uint8_t vga_entry_color(enum vga_color fg, enum vga_color bg)
    {
        return fg | bg << 4;
    }

    /// @brief  Combines a character with a color attribute to form a VGA text mode entry.
    /// @param uc The character.
    /// @param color The color attribute.
    /// @return The VGA text mode entry formed by combining the character with the color attribute.
    static inline uint16_t vga_entry(unsigned char uc, uint8_t color)
    {
        return (uint16_t)uc | (uint16_t)color << 8;
    }

#if defined(__cplusplus)
} /* extern "C" */
#endif
#endif
