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
#ifndef B8E0EF46_0B01_45A0_8AA2_0B2B4992066F
#define B8E0EF46_0B01_45A0_8AA2_0B2B4992066F

#include <stdint.h>

/**
 * @brief Namespace representing the Global Descriptor Table (GDT).
 *
 * The GDT namespace encapsulates the GDT table and its operations. It provides functions
 * for initializing the GDT, setting entries, and loading the GDT into the CPU.
 */
namespace GDT
{

    /// @brief Struct representing the access field in a GDT entry (8bit).
    struct access_field_t
    {
        /// @brief Accessed bit.
        enum class Accessed_Bit : uint8_t
        {
            NotAccessed = 0, /**< NotAccessed*/
            Accessed = 1,    /**< Accessed */
        };

        /// @brief Readable bit or Writable bit
        enum class RW_Bit : uint8_t
        {
            Reset = 0, /**< RW is 0 */
            Set = 1    /**< RW is 1 */
        };

        /// @brief  Direction Bit/Conforming Bit.
        enum class Direction_Bit : uint8_t
        {
            GrowsUp = 0,  /**< the segment grows up */
            GrowsDown = 1 /**<  the segment grows down */
        };

        /// @brief Executable Bit
        enum class Executable_Bit : uint8_t
        {
            NotExecutable = 0, /**< data selector */
            Executable = 1     /**< code selector */
        };

        /// @brief Descriptor type
        enum class Descriptor_Type : uint8_t
        {
            SystemSegment = 0,    /**< system segments */
            CodeOrDataSegment = 1 /**< code/data segments */
        };

        /// @brief Privilege flag
        enum class Privilege_Flag : uint8_t
        {
            Highest = 0, /**< highest(kernel) */
            Lowest = 3   /**< lowest(user) */
        };

        /// @brief Present Bit
        enum class Present_Bit : uint8_t
        {
            Reset = 0, /**< 0 for null sector*/
            Set = 1    /**< 1 for all valid sectors */
        };

        Accessed_Bit accessed : 1; /**< accessed  (1 bit). set 0 */

        RW_Bit RW_flag : 1; /**< Readable bit or Writable bit.  (1 bit).
                          For code selectors, the readable bit indicates that read access is permitted for this segment.
                          Write access is never allowed for code segments.
                          For data selectors, the writable bit signifies that write access is permitted for this segment.
                          Read access is always allowed for data segments.*/

        Direction_Bit direction : 1; /**< Direction  (1 bit).*/

        Executable_Bit executable : 1; /**< Executable type (1 bit). */

        Descriptor_Type descriptor : 1; /**< Descriptor type (1 bit).*/

        Privilege_Flag privilege : 2; /**< Privilege flag (2 bit).*/

        Present_Bit present : 1; /**< Segment present flag (1 bit) */

        access_field_t() = default;

        /**
         * @brief Convert access_field_t struct to uint8_t value
         *
         * @return uint8_t The converted 8-bit value representing the access_field_t struct
         */
        uint8_t ConertToUint8() const;
    };

    /// @brief Struct representing the flags field in a GDT entry.
    struct flags_field_t
    {
        /// @brief Size bit
        enum class Size : uint8_t
        {
            _16BitProtected = 0, /**< 16 bit protected mode */
            _32BitProtected = 1  /**< 32 bit protected mode */
        };

        /// @brief Granularity bit
        enum class Granularity : uint8_t
        {
            ByteGranularity = 0, /**< limit is in 1 B blocks */
            PageGranularity = 1  /**< limit is in 4 KiB blocks */
        };

        Size sz : 1;        /**< Granularity bit (1 bit). */
        Granularity gr : 1; /**< Size bit(1 bit). */

        flags_field_t() = default;

        /**
         * @brief Convert flags_field_t struct to uint8_t value
         *
         * @return uint8_t The converted 8-bit value representing the flags_field_t struct
         */
        uint8_t ConertToUint8() const;
    };

    /// @brief Represents a GDT entry.
    struct GDTEntry
    {
        uint16_t limit_low;           /**< Limit 0:15 */
        uint16_t base_low;            /**< Base 0:15 */
        uint8_t base_middle;          /**< Base 16:23 */
        uint8_t access;               /**< empty (2 bit). */
        uint8_t limit_high_and_flags; /**< Limit 16:19 and Flags */
        uint8_t base_high;            /**< Base 24:31 */
    } __attribute__((packed));

    /// @brief Represents the Global Descriptor Table Register (GDTR).
    struct GDTR
    {
        uint16_t limit; ///< Size of the GDT - 1
        uint32_t base;  ///< Base address of the GDT
    } __attribute__((packed));

    /**
     * @brief Initializes the Global Descriptor Table (GDT).
     *
     * Sets up the GDT by defining entries for null, code, and data segments.
     * Loads the GDT into the CPU using the `lgdt` instruction.
     */
    void Init();

};

#endif /* B8E0EF46_0B01_45A0_8AA2_0B2B4992066F */
