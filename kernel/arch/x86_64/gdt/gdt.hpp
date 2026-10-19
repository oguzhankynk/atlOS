/**
 * @file gdt.hpp
 * @author Oguzhan KAYNAK ()
 * @brief Header file for Global Descriptor Table (GDT) structures and functions.
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

/**
 * @brief Namespace representing the Global Descriptor Table (GDT).
 *
 * The GDT namespace encapsulates the GDT table and its operations. It provides functions
 * for initializing the GDT, setting entries, and loading the GDT into the CPU.
 */
namespace GDT {

/// @brief Represents a GDT entry.
struct GDTEntry {
    uint16_t limit_low; /**< Limit 0:15 */
    uint16_t base_low; /**< Base 0:15 */
    uint8_t base_middle; /**< Base 16:23 */
    uint8_t access; /**< empty (2 bit). */
    uint8_t limit_high_and_flags; /**< Limit 16:19 and Flags */
    uint8_t base_high; /**< Base 24:31 */
} __attribute__((packed));

/// @brief Represents the Global Descriptor Table Register (GDTR).
struct GDTR {
    uint16_t limit; ///< Size of the GDT - 1
    uintptr_t base; ///< Base address of the GDT
} __attribute__((packed));

/**
 * @brief Returns a reference to the GDTR.
 *
 * @return const GDTR&
 */
const GDTR& getGDTR();

};
