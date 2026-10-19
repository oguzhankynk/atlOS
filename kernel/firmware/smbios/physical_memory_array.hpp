/**
 * @file physical_memory_array.hpp
 * @author Oguzhan KAYNAK
 * @brief Reading the Physical Memory Array (Type 16) structure field in SMBIOS
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

/// @brief Physical Memory Array Type 16 structure class.
class PhysicalMemoryArray : public BIOSStructureCommon {
public:
    /// @brief Default constructor.
    PhysicalMemoryArray() = default;

    /// @brief Constructor that initializes by scanning the given address.
    /// @param pStart Start address of the structure.
    PhysicalMemoryArray(void* pStart)
    {
        scan(pStart);
    }

    /// @brief Scans the structure starting from the given address.
    /// @param pStart Start address of the structure.
    /// @return End address of the scanned structure.
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

    /// @brief Gets the type of the physical memory array.
    /// @return Type of the physical memory array.
    uint8_t getType() const { return *getTypeAddr(); }

    /// @brief Gets the length of the structure.
    /// @return Length of the structure.
    uint8_t getLength() const { return *getLengthAddr(); }

    /// @brief Gets the handle associated with the structure.
    /// @return Handle associated with the structure.
    uint16_t getHandle() const { return *getHandleAddr(); }

    /// @brief Gets the location of the memory array.
    /// @return Location of the memory array.
    uint8_t getLocation() const { return *getLocationAddr(); }

    /// @brief Gets the use of the memory array.
    /// @return Use of the memory array.
    uint8_t getUse() const { return *getUseAddr(); }

    /// @brief Gets the memory error correction information.
    /// @return Memory error correction information.
    uint8_t getMemoryErrorCorrection() const { return *getMemoryErrorCorrectionAddr(); }

    /// @brief Gets the maximum capacity of the memory array.
    /// @return Maximum capacity of the memory array.
    uint32_t getMaximumCapacity() const { return *getMaximumCapacityAddr(); }

    /// @brief Gets the memory error information handle.
    /// @return Memory error information handle.
    uint16_t getMemoryErrorInformationHandle() const { return *getMemoryErrorInformationHandleAddr(); }

    /// @brief Gets the number of memory devices.
    /// @return Number of memory devices.
    uint16_t getNumberOfMemoryDevices() const { return *getNumberOfMemoryDevicesAddr(); }

    /// @brief Gets the extended maximum capacity of the memory array.
    /// @return Extended maximum capacity of the memory array.
    uint64_t getExtendedMaximumCapacity() const { return *getExtendedMaximumCapacityAddr(); }

    /// @brief Prints the content of the Physical Memory Array structure.
    void print() const override
    {
        printf("Type: %u\n", getType());
        printf("Length: %u\n", getLength());
        printf("Handle: %u\n", getHandle());
        printf("Location: %u\n", getLocation());
        printf("Use: %u\n", getUse());
        printf("Memory Error Correction: %u\n", getMemoryErrorCorrection());
        printf("Maximum Capacity: %u KB\n", getMaximumCapacity());
        printf("Memory Error Information Handle: %u\n", getMemoryErrorInformationHandle());
        printf("Number of Memory Devices: %u\n", getNumberOfMemoryDevices());
        printf("Extended Maximum Capacity: %llu bytes\n", getExtendedMaximumCapacity());
    }

private:
    void* m_pStart {}; ///< Start address of the structure.
    void* m_pEnd {}; ///< End address of the structure.

    // Private methods to get the addresses of different fields.

    /// @brief Gets the address of the Type field.
    /// @return Pointer to the Type field.
    uint8_t* getTypeAddr() const { return reinterpret_cast<uint8_t*>(reinterpret_cast<uint8_t*>(m_pStart) + 0x00); }

    /// @brief Gets the address of the Length field.
    /// @return Pointer to the Length field.
    uint8_t* getLengthAddr() const { return reinterpret_cast<uint8_t*>(reinterpret_cast<uint8_t*>(m_pStart) + 0x01); }

    /// @brief Gets the address of the Handle field.
    /// @return Pointer to the Handle field.
    uint16_t* getHandleAddr() const { return reinterpret_cast<uint16_t*>(reinterpret_cast<uint8_t*>(m_pStart) + 0x02); }

    /// @brief Gets the address of the Location field.
    /// @return Pointer to the Location field.
    uint8_t* getLocationAddr() const { return reinterpret_cast<uint8_t*>(reinterpret_cast<uint8_t*>(m_pStart) + 0x04); }

    /// @brief Gets the address of the Use field.
    /// @return Pointer to the Use field.
    uint8_t* getUseAddr() const { return reinterpret_cast<uint8_t*>(reinterpret_cast<uint8_t*>(m_pStart) + 0x05); }

    /// @brief Gets the address of the Memory Error Correction field.
    /// @return Pointer to the Memory Error Correction field.
    uint8_t* getMemoryErrorCorrectionAddr() const { return reinterpret_cast<uint8_t*>(reinterpret_cast<uint8_t*>(m_pStart) + 0x06); }

    /// @brief Gets the address of the Maximum Capacity field.
    /// @return Pointer to the Maximum Capacity field.
    uint32_t* getMaximumCapacityAddr() const { return reinterpret_cast<uint32_t*>(reinterpret_cast<uint8_t*>(m_pStart) + 0x07); }

    /// @brief Gets the address of the Memory Error Information Handle field.
    /// @return Pointer to the Memory Error Information Handle field.
    uint16_t* getMemoryErrorInformationHandleAddr() const { return reinterpret_cast<uint16_t*>(reinterpret_cast<uint8_t*>(m_pStart) + 0x0B); }

    /// @brief Gets the address of the Number of Memory Devices field.
    /// @return Pointer to the Number of Memory Devices field.
    uint16_t* getNumberOfMemoryDevicesAddr() const { return reinterpret_cast<uint16_t*>(reinterpret_cast<uint8_t*>(m_pStart) + 0x0D); }

    /// @brief Gets the address of the Extended Maximum Capacity field.
    /// @return Pointer to the Extended Maximum Capacity field.
    uint64_t* getExtendedMaximumCapacityAddr() const { return reinterpret_cast<uint64_t*>(reinterpret_cast<uint8_t*>(m_pStart) + 0x0F); }
};
