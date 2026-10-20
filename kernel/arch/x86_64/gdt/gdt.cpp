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

#include <arch/x86_64/gdt/gdt.hpp>

#include <arch/x86_64/cpu/cpu.hpp>

namespace GDT {
/**
 * @brief Global Descriptor Table (GDT) entries.
 *
 */
__attribute__((section(".bootdata"), aligned(0x1000))) static GDTEntry s_gdtTable[7] = {

    { // 0: NULL
        0, 0, 0, 0, 0, 0

    },

    { // 1: Kernel code segment
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

    { // 2: Kernel data segment (DS, SS)
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

    { // 3: User code segment (Ring 3)
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

    { // 4: User data segment (Ring 3)
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

    },

    // 5, 6: TSS descriptor (selector 0x28). A system descriptor is 16 bytes in
    // long mode, so it takes two slots; init() fills it in, the TSS address is
    // not known before the kernel is linked.
    { 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0 }

};

extern "C" {
/**
 * @brief Global Descriptor Table Register (GDTR) structure. boot.S will load this.
 *
 */
__attribute__((section(".bootdata"), used)) GDTR gdt_ptr = {
    sizeof(s_gdtTable) - 1,
    reinterpret_cast<uint64_t>(&s_gdtTable)
};
}

/**
 * @brief The TSS. One CPU, one TSS; a CPU's TSS is hardware state like the IDT,
 * so it is a static table rather than an object.
 */
static TaskStateSegment s_tss;

/**
 * @brief Stack of the double fault handler. A double fault often means the
 * current stack is broken (a kernel stack overflow), so the CPU switches to
 * this one through IST1 before pushing anything.
 */
alignas(16) static uint8_t s_doubleFaultStack[doubleFaultStackSize];

void init()
{
    // The stack grows down: IST1 is its top
    s_tss.ist[doubleFaultStack - 1] = reinterpret_cast<uint64_t>(s_doubleFaultStack + doubleFaultStackSize);
    s_tss.ioMapBase = sizeof(TaskStateSegment);

    // 64-bit TSS descriptor (Intel SDM Vol. 3A, "TSS Descriptor in 64-bit mode"):
    // a GDTEntry with type 9 (available 64-bit TSS), followed by base 32..63
    uint64_t base = reinterpret_cast<uint64_t>(&s_tss);
    uint32_t limit = sizeof(TaskStateSegment) - 1;
    GDTEntry& low = s_gdtTable[static_cast<uint16_t>(SegmentSelect::TSS) / sizeof(GDTEntry)];
    GDTEntry& high = (&low)[1];

    low.limitLow = limit & 0xFFFF;
    low.baseLow = base & 0xFFFF;
    low.baseMiddle = (base >> 16) & 0xFF;
    low.access = 0x89; // P=1, DPL=0, type 9
    low.limitHighAndFlags = (limit >> 16) & 0x0F; // G=0: the limit is in bytes
    low.baseHigh = (base >> 24) & 0xFF;

    high.limitLow = (base >> 32) & 0xFFFF;
    high.baseLow = (base >> 48) & 0xFFFF;

    // The GDTR already points at the table (boot.S), loading TR reads the new descriptor
    __asm__ volatile("ltr %0"
                     :
                     : "r"(static_cast<uint16_t>(SegmentSelect::TSS)));
}

const TaskStateSegment& taskStateSegment()
{
    return s_tss;
}

/**
 * @brief Returns a reference to the GDTR structure.
 *
 * @return const GDTR&
 */
const GDTR& getGDTR()
{
    return gdt_ptr;
}

}