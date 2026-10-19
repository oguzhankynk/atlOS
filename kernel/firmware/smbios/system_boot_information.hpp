/**
 * @file system_boot_information.hpp
 * @author Oguzhan KAYNAK
 * @brief Reading the System Boot Information (Type 32)field in SMBIOS
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

#include <firmware/bios/structure_common.hpp>
#include <stdint.h>
#include <stdio.h>

/// @brief Class to represent the System Boot Information table (Type 32).
class SystemBootInformation : public BIOSStructureCommon {
public:
    /// @brief Default constructor.
    SystemBootInformation() = default;

    /// @brief Constructor that takes a pointer to the start of the structure.
    /// @param pStart Pointer to the start of the structure.
    SystemBootInformation(void* pStart)
    {
        scan(pStart);
    }

    /// @brief Scans the System Boot Information table.
    /// @param pStart Pointer to the start of the structure.
    /// @return Pointer to the end of the structure.
    void* scan(void* pStart) override
    {
        m_pStart = pStart;

        int end = 1;
        uint8_t* ptr = (uint8_t*)m_pStart + getLength();

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

    /// @brief Gets the Boot Status value.
    /// @return The Boot Status value.
    uint8_t getBootStatus() const { return *getBootStatusAddr(); }

    // Print function to display all values
    void print() const override
    {
        printf("Type: %u\n", getType());
        printf("Length: %u\n", getLength());
        printf("Handle: %u\n", getHandle());
        printf("Boot Status: %u\n", getBootStatus());
    }

private:
    void* m_pStart {}; ///< Start address of the structure.
    void* m_pEnd {}; ///< End address of the structure.

    // Private methods to get the addresses of different fields.

    /// @brief Gets the address of the Type field.
    /// @return Pointer to the Type field.
    uint8_t* getTypeAddr() const { return reinterpret_cast<uint8_t*>(m_pStart) + 0x00; }

    /// @brief Gets the address of the Length field.
    /// @return Pointer to the Length field.
    uint8_t* getLengthAddr() const { return reinterpret_cast<uint8_t*>(m_pStart) + 0x01; }

    /// @brief Gets the address of the Handle field.
    /// @return Pointer to the Handle field.
    uint16_t* getHandleAddr() const { return reinterpret_cast<uint16_t*>(reinterpret_cast<uint8_t*>(m_pStart) + 0x02); }

    /// @brief Gets the address of the Boot Status field.
    /// @return Pointer to the Boot Status field.
    uint8_t* getBootStatusAddr() const { return reinterpret_cast<uint8_t*>(reinterpret_cast<uint8_t*>(m_pStart) + 0x0A); }
};
