/**
 * @file gdt.cpp
 * @author Oguzhan KAYNAK ()
 * @brief Implementation file for the GDT class and its functions.
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

#include <arch/x86_64/GDT/gdt.hpp>

namespace GDT
{
    /**
     * @brief Global Descriptor Table (GDT) entries.
     *
     */
    __attribute__((section(".bootdata"), aligned(0x1000))) static GDTEntry gdt_table[5] = {

        {// 0: NULL
         0, 0, 0, 0, 0, 0

        },

        {// 1: Kernel code segment
         // P=1       : Segment present
         // DPL=0     : Ring0 (highest privilege)
         // S=1       : Code/Data segment
         // E=1       : Executable (code segment)
         // DC=0      : Non-conforming
         // R=1       : Readable
         // A=0       : Not accessed yet
         // flags=0xA0:
         // L=1       : 64-bit segment
         // D/B=0     : Default size ignored in 64-bit
         // G=1       : Granularity 4 KB
         0xFFFF, 0x0000, 0x00, 0x9A, 0xA0, 0x00

        },

        {// 2: Kernel data segment (DS, SS)
         // P=1       : Segment present
         // DPL=0     : Ring0
         // S=1       : Code/Data segment
         // E=0       : Data segment
         // DC=0      : Grow up
         // R/W=1     : Writable
         // A=0       : Not accessed
         // flags=0xA0:
         // L=1       : 64-bit segment (ignored for data)
         // D/B=0     : Default size
         // G=1       : 4 KB granularity
         0xFFFF, 0x0000, 0x00, 0x92, 0xA0, 0x00

        },

        {// 3: User code segment (Ring 3)
         // P=1       : Segment present
         // DPL=3     : Ring3 (user)
         // S=1       : Code/Data
         // E=1       : Executable
         // DC=0      : Non-conforming
         // R=1       : Readable
         // A=0       : Not accessed
         // flags=0xA0:
         // L=1       : 64-bit code
         // G=1       : 4 KB granularity
         0xFFFF, 0x0000, 0x00, 0xFA, 0xA0, 0x00

        },

        {// 4: User data segment (Ring 3)
         // P=1       : Segment present
         // DPL=3     : Ring3
         // S=1       : Code/Data
         // E=0       : Data segment
         // DC=0      : Grow up
         // R/W=1     : Writable
         // A=0       : Not accessed
         // flags=0xA0:
         // L=1       : 64-bit segment (ignored for data)
         // G=1       : 4 KB granularity
         0xFFFF, 0x0000, 0x00, 0xF2, 0xA0, 0x00

        }};

    extern "C"
    {
        /**
         * @brief Global Descriptor Table Register (GDTR) structure. boot.S will load this.
         *
         */
        __attribute__((section(".bootdata"))) GDTR gdt_ptr = {
            sizeof(gdt_table) - 1,
            reinterpret_cast<uint64_t>(&gdt_table)};
    }

    /**
     * @brief Returns a reference to the GDTR structure.
     *
     * @return const GDTR&
     */
    const GDTR &getGDTR()
    {
        return gdt_ptr;
    }

}