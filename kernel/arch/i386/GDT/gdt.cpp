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

#include <arch/i386/GDT/gdt.hpp>
#include <arch/i386/cpu/cpu.hpp>

namespace GDT
{
    static constexpr uint8_t GDT_TABLE_LEN = 5; /* NULL, Kernel Code, Kernel Data, User Code, User Data, TSS*/

    // GDT table (array of GDT entries)
    GDTEntry gdtEntries[GDT_TABLE_LEN]{}; /* Null descriptor init*/

    // GDTR structure (Global Descriptor Table Register)
    GDTR gdtr;

    static void setEntry(int index, uint32_t base, uint32_t limit, uint8_t access, uint8_t granularity);
    static void loadAndUpdateSegmentRegisters();

    /**
     * @brief Convert access_field_t struct to uint8_t value
     *
     * @return uint8_t The converted 8-bit value representing the access_field_t struct
     */
    uint8_t access_field_t::ConertToUint8() const
    {
        uint8_t result = 0;
        result |= ((uint8_t)present << 7);
        result |= ((uint8_t)privilege << 5);
        result |= ((uint8_t)descriptor << 4);
        result |= ((uint8_t)executable << 3);
        result |= ((uint8_t)direction << 2);
        result |= ((uint8_t)RW_flag << 1);
        result |= ((uint8_t)accessed << 0);
        return result;
    }

    uint8_t flags_field_t::ConertToUint8() const
    {
        uint8_t result = 0;
        result |= ((uint8_t)gr << 7);
        result |= ((uint8_t)sz << 6);
        result |= 0x0F;
        return result;
    }

    /**
     * @brief Initialize the Global Descriptor Table (GDT)
     *
     */
    void Init()
    {

        setEntry(0, 0, 0, 0, 0);

        // kernel Code segment descriptor
        setEntry(1, 0, 0xFFFFF,

                 access_field_t{access_field_t::Accessed_Bit::NotAccessed, access_field_t::RW_Bit::Set, access_field_t::Direction_Bit::GrowsUp,
                                access_field_t::Executable_Bit::Executable, access_field_t::Descriptor_Type::CodeOrDataSegment,
                                access_field_t::Privilege_Flag::Highest, access_field_t::Present_Bit::Set}
                     .ConertToUint8(),

                 flags_field_t{flags_field_t::Size::_32BitProtected, flags_field_t::Granularity::PageGranularity}.ConertToUint8());

        // kernel Code Data descriptor
        setEntry(2, 0, 0xFFFFF,

                 access_field_t{access_field_t::Accessed_Bit::NotAccessed, access_field_t::RW_Bit::Set, access_field_t::Direction_Bit::GrowsUp,
                                access_field_t::Executable_Bit::NotExecutable, access_field_t::Descriptor_Type::CodeOrDataSegment,
                                access_field_t::Privilege_Flag::Highest, access_field_t::Present_Bit::Set}
                     .ConertToUint8(),

                 flags_field_t{flags_field_t::Size::_32BitProtected, flags_field_t::Granularity::PageGranularity}.ConertToUint8());

        // User Code segment descriptor
        setEntry(3, 0, 0xFFFFF,

                 access_field_t{access_field_t::Accessed_Bit::NotAccessed, access_field_t::RW_Bit::Set, access_field_t::Direction_Bit::GrowsUp,
                                access_field_t::Executable_Bit::Executable, access_field_t::Descriptor_Type::CodeOrDataSegment,
                                access_field_t::Privilege_Flag::Lowest, access_field_t::Present_Bit::Set}
                     .ConertToUint8(),

                 flags_field_t{flags_field_t::Size::_32BitProtected, flags_field_t::Granularity::PageGranularity}.ConertToUint8());

        // User Code Data descriptor
        setEntry(4, 0, 0xFFFFF,

                 access_field_t{access_field_t::Accessed_Bit::NotAccessed, access_field_t::RW_Bit::Set, access_field_t::Direction_Bit::GrowsUp,
                                access_field_t::Executable_Bit::NotExecutable, access_field_t::Descriptor_Type::CodeOrDataSegment,
                                access_field_t::Privilege_Flag::Lowest, access_field_t::Present_Bit::Set}
                     .ConertToUint8(),

                 flags_field_t{flags_field_t::Size::_32BitProtected, flags_field_t::Granularity::PageGranularity}.ConertToUint8());

        // TODO TSS

        // Set the GDTR structure
        gdtr.limit = sizeof(gdtEntries) - 1;
        gdtr.base = (uint32_t)&gdtEntries;

        loadAndUpdateSegmentRegisters();
    }

    /**
     * @brief Sets up a GDT entry.
     *
     * @param index The index of the GDT entry to set.
     * @param base The base address of the segment.
     * @param limit The limit (size) of the segment.
     * @param access The access flags for the segment.
     * @param flags The granularity.
     */
    static void setEntry(int index, uint32_t base, uint32_t limit, uint8_t access, uint8_t granularity)
    {
        // Set the base and limit
        gdtEntries[index].limit_low = limit & 0xFFFF;
        gdtEntries[index].base_low = base & 0xFFFF;
        gdtEntries[index].base_middle = (base >> 16) & 0xFF;
        gdtEntries[index].access = access;
        gdtEntries[index].limit_high_and_flags = (limit >> 16) & 0x0F;
        gdtEntries[index].limit_high_and_flags |= granularity & 0xF0;
        gdtEntries[index].base_high = (base >> 24) & 0xFF;
    }

    /**
     * @brief Loads the GDT into the CPU and updates segment registers.
     *
     * This method uses the `lgdt` instruction to load the Global Descriptor Table (GDT) into the
     * CPU's GDTR register, effectively setting up the memory segmentation for the system.
     * After loading the GDT, the method updates the segment registers (DS, ES, FS, GS, SS)
     * using the provided code and data segment selectors.
     *
     * The method performs a far jump (`ljmp`) to update the code segment register (CS).
     * It then sets the other segment registers using the data segment selector.
     *
     */
    static void loadAndUpdateSegmentRegisters()
    {
        uint32_t code = static_cast<uint32_t>(SegmentSelect::KERNELCODE);
        uint32_t data = static_cast<uint32_t>(SegmentSelect::KERNELDATA);

        __asm__ __volatile__(
            "lgdt %0\n"
            "pushl %1\n"
            "pushl $reload_CS\n"
            "lret\n"
            "reload_CS:\n"
            "mov %2,%%eax\n"
            "mov %%ax,%%ds\n"
            "mov %%ax,%%es\n"
            "mov %%ax,%%fs\n"
            "mov %%ax,%%gs\n"
            "mov %%ax,%%ss\n"
            :
            : "g"(gdtr), "r"(code), "r"(data));
    }

}; // namespace GDT