/**
 * @file end_of_table.hpp
 * @author Oguzhan KAYNAK
 * @brief Reading the End-of-Table (Type 127) structure field in SMBIOS
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
#ifndef F47859F0_E52E_4F1C_A004_C351DB461B90
#define F47859F0_E52E_4F1C_A004_C351DB461B90

#include <stdint.h>
#include <stdio.h>
#include <firmware/bios/structure_common.hpp>

/// @brief Class to represent the End-of-Table structure.
class EndOfTable : public BIOSStructureCommon
{
public:
    /// @brief Default constructor.
    EndOfTable() = default;

    /// @brief Constructor that takes a pointer to the start of the structure.
    /// @param p_start Pointer to the start of the structure.
    EndOfTable(void *p_start)
    {
        Scan(p_start);
    }

    /// @brief Scans the End-of-Table structure.
    /// @param p_start Pointer to the start of the structure.
    /// @return Pointer to the end of the structure.
    void *Scan(void *p_start) override
    {
        m_pStart = p_start;

        int end = 1;
        uint8_t *ptr = (uint8_t *)m_pStart + getLength();

        for (; ptr[end - 1] != '\0' || ptr[end] != '\0'; end++)
            ;

        m_pEnd = ptr + end + 1;

        return m_pEnd;
    }

    // Getter methods for accessing the values of different fields.

    /// @brief Gets the Type value.
    /// @return The Type value.
    uint8_t getType() const { return *getTypeAddr(); }

    /// @brief Gets the Length value.
    /// @return The Length value.
    uint8_t getLength() const { return *getLengthAddr(); }

    /// @brief Gets the Handle value.
    /// @return The Handle value.
    uint16_t getHandle() const { return *getHandleAddr(); }

    // Print function to display all values
    void print() const override
    {
        printf("Type: %u\n", getType());
        printf("Length: %u\n", getLength());
        printf("Handle: %u\n", getHandle());
    }

private:
    // Private methods to get the addresses of different fields.
    void *m_pStart{}; ///< Start address of the structure.
    void *m_pEnd{};   ///< End address of the structure.

    /// @brief Gets the address of the Type field.
    /// @return Pointer to the Type field.
    uint8_t *getTypeAddr() const { return reinterpret_cast<uint8_t *>(m_pStart) + 0x00; }

    /// @brief Gets the address of the Length field.
    /// @return Pointer to the Length field.
    uint8_t *getLengthAddr() const { return reinterpret_cast<uint8_t *>(m_pStart) + 0x01; }

    /// @brief Gets the address of the Handle field.
    /// @return Pointer to the Handle field.
    uint16_t *getHandleAddr() const { return reinterpret_cast<uint16_t *>(reinterpret_cast<uint8_t *>(m_pStart) + 0x02); }
};

#endif /* F47859F0_E52E_4F1C_A004_C351DB461B90 */
