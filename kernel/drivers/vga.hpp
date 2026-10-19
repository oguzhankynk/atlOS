/**
 * @file vga.hpp
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
    enum VGAColor
    {
        VgaColorBlack = 0,          ///< Black color (0).
        VgaColorBlue = 1,           ///< Blue color (1).
        VgaColorGreen = 2,          ///< Green color (2).
        VgaColorCyan = 3,           ///< Cyan color (3).
        VgaColorRed = 4,            ///< Red color (4).
        VgaColorMagenta = 5,        ///< Magenta color (5).
        VgaColorBrown = 6,          ///< Brown color (6).
        VgaColorLightGrey = 7,     ///< Light grey color (7).
        VgaColorDarkGrey = 8,      ///< Dark grey color (8).
        VgaColorLightBlue = 9,     ///< Light blue color (9).
        VgaColorLightGreen = 10,   ///< Light green color (10).
        VgaColorLightCyan = 11,    ///< Light cyan color (11).
        VgaColorLightRed = 12,     ///< Light red color (12).
        VgaColorLightMagenta = 13, ///< Light magenta color (13).
        VgaColorLightBrown = 14,   ///< Light brown color (14).
        VgaColorWhite = 15          ///< White color (15).
    };

    /// @brief Combines foreground and background colors to generate a VGA text mode color attribute.
    /// @param fg The foreground color.
    /// @param bg The background color.
    /// @return  The VGA text mode color attribute formed by combining the foreground and background colors.
    static inline uint8_t vgaEntryColor(enum VGAColor fg, enum VGAColor bg)
    {
        return fg | bg << 4;
    }

    /// @brief  Combines a character with a color attribute to form a VGA text mode entry.
    /// @param uc The character.
    /// @param color The color attribute.
    /// @return The VGA text mode entry formed by combining the character with the color attribute.
    static inline uint16_t vgaEntry(unsigned char uc, uint8_t color)
    {
        return (uint16_t)uc | (uint16_t)color << 8;
    }

#if defined(__cplusplus)
} /* extern "C" */
#endif
#endif
