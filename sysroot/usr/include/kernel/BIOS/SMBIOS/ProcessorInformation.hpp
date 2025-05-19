/**
 * @file ProcessorInformation.hpp
 * @author Oguzhan KAYNAK
 * @brief Reading the Processor Information (Type 4) structure field in SMBIOS
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
#ifndef E783A862_53F3_4C99_B779_8123301B75AE
#define E783A862_53F3_4C99_B779_8123301B75AE

#include <stdint.h>
#include <stdio.h>
#include "kernel/BIOS/BIOSStructureCommon.hpp"

/// @brief Class for handling Processor Information (Type 4) structure in SMBIOS.
class ProcessorInformation : public BIOSStructureCommon
{
public:
    ProcessorInformation() = default;
    ProcessorInformation(void *p_start)
    {
        Scan(p_start);
    }

    /// @brief Scans the Processor Information structure starting from the given address.
    /// @param p_start Pointer to the start address of the Processor Information structure.
    /// @return Pointer to the end address of the structure.
    void *Scan(void *p_start) override
    {

        m_pStart = p_start;

        int end = 1;
        uint8_t *ptr = (uint8_t *)m_pStart + getLength();

        for (; ptr[end - 1] != '\0' || ptr[end] != '\0'; end++)
            ;

        m_pEnd = ptr + end + 1;

        extractStringAddress((const char *)ptr, (const char *)ptr + end, m_strings, StringCount);

        return m_pEnd;
    }

    // Getter methods for retrieving information

    /// @brief Gets the type of the structure.
    uint8_t getType() const { return *getTypeAddr(); }

    /// @brief Gets the length of the structure.
    uint8_t getLength() const { return *getLengthAddr(); }

    /// @brief Gets the handle of the structure.
    uint16_t getHandle() const { return *getHandleAddr(); }

    /// @brief Gets the socket designation.
    uint8_t getSocketDesignation() const { return *getSocketDesignationAddr(); }

    /// @brief Gets the processor type.
    uint8_t getProcessorType() const { return *getProcessorTypeAddr(); }

    /// @brief Gets the processor family.
    uint8_t getProcessorFamily() const { return *getProcessorFamilyAddr(); }

    /// @brief Gets the processor manufacturer.
    uint8_t getProcessorManufacturer() const { return *getProcessorManufacturerAddr(); }

    /// @brief Gets the processor ID.
    uint32_t getProcessorID() const { return *getProcessorIDAddr(); }

    /// @brief Gets the processor version.
    uint8_t getProcessorVersion() const { return *getProcessorVersionAddr(); }

    /// @brief Gets the voltage of the processor.
    uint8_t getVoltage() const { return *getVoltageAddr(); }

    /// @brief Gets the external clock speed.
    uint16_t getExternalClock() const { return *getExternalClockAddr(); }

    /// @brief Gets the maximum speed of the processor.
    uint16_t getMaxSpeed() const { return *getMaxSpeedAddr(); }

    /// @brief Gets the current speed of the processor.
    uint16_t getCurrentSpeed() const { return *getCurrentSpeedAddr(); }

    /// @brief Gets the status of the processor.
    uint8_t getStatus() const { return *getStatusAddr(); }

    /// @brief Gets the processor upgrade information.
    uint8_t getProcessorUpgrade() const { return *getProcessorUpgradeAddr(); }

    /// @brief Gets the L1 cache handle.
    uint16_t getL1CacheHandle() const { return *getL1CacheHandleAddr(); }

    /// @brief Gets the L2 cache handle.
    uint16_t getL2CacheHandle() const { return *getL2CacheHandleAddr(); }

    /// @brief Gets the L3 cache handle.
    uint16_t getL3CacheHandle() const { return *getL3CacheHandleAddr(); }

    /// @brief Gets the serial number of the processor.
    uint8_t getSerialNumber() const { return *getSerialNumberAddr(); }

    /// @brief Gets the asset tag number of the processor.
    uint8_t getAssetTag() const { return *getAssetTagAddr(); }

    /// @brief Gets the part number of the processor.
    uint8_t getPartNumber() const { return *getPartNumberAddr(); }

    /// @brief Gets the core count of the processor.
    uint8_t getCoreCount() const { return *getCoreCountAddr(); }

    /// @brief Gets the number of enabled cores.
    uint8_t getCoreEnabled() const { return *getCoreEnabledAddr(); }

    /// @brief Gets the number of threads.
    uint8_t getThreadCount() const { return *getThreadCountAddr(); }

    /// @brief Gets the processor characteristics.
    uint16_t getProcessorCharacteristics() const { return *getProcessorCharacteristicsAddr(); }

    /// @brief Gets the secondary processor family.
    uint16_t getProcessorFamily2() const { return *getProcessorFamily2Addr(); }

    /// @brief Prints the extracted information of the system structure.
    void print() const override
    {
        printf("Type: %u\n", getType());
        printf("Length: %u\n", getLength());
        printf("Handle: %u\n", getHandle());
        printf("Socket Designation: %u\n", getSocketDesignation());
        printf("Processor Type: %u\n", getProcessorType());
        printf("Processor Family: %u\n", getProcessorFamily());
        printf("Processor Manufacturer: %u\n", getProcessorManufacturer());
        printf("Processor ID: %u\n", getProcessorID());
        printf("Processor Version: %u\n", getProcessorVersion());
        printf("Voltage: %u\n", getVoltage());
        printf("External Clock: %u\n", getExternalClock());
        printf("Max Speed: %u\n", getMaxSpeed());
        printf("Current Speed: %u\n", getCurrentSpeed());
        printf("Status: %u\n", getStatus());
        printf("Processor Upgrade: %u\n", getProcessorUpgrade());
        printf("L1 Cache Handle: %u\n", getL1CacheHandle());
        printf("L2 Cache Handle: %u\n", getL2CacheHandle());
        printf("L3 Cache Handle: %u\n", getL3CacheHandle());
        printf("Serial Number: %u\n", getSerialNumber());
        printf("Asset Tag: %u\n", getAssetTag());
        printf("Part Number: %u\n", getPartNumber());
        printf("Core Count: %u\n", getCoreCount());
        printf("Core Enabled: %u\n", getCoreEnabled());
        printf("Thread Count: %u\n", getThreadCount());
        printf("Processor Characteristics: %u\n", getProcessorCharacteristics());
        printf("Processor Family 2: %u\n", getProcessorFamily2());
        for (size_t i = 0; i < StringCount; ++i)
        {
            printf("[%d]:%s ", i, m_strings[i]);
        }
        printf("\n");
    }

private:
    void *m_pStart{}; ///< Start address of the structure.
    void *m_pEnd{};   ///< End address of the structure.

    /*
        Socket Designation String;
        Processor Manufacturer String;
        Processor Version String;
        Serial Number String;
        Asset Tag String;
        Part Number String;
    */
    static constexpr uint8_t StringCount = 6; ///< Number of string fields.
    const char *m_strings[StringCount]{};     ///< Array to store string fields.

    // Private methods to find addresses of structure fields

    /// @brief Gets the address of the type field.
    uint8_t *getTypeAddr() const { return (uint8_t *)(static_cast<uint8_t *>(m_pStart) + 0x00); }

    /// @brief Gets the address of the length field.
    uint8_t *getLengthAddr() const { return (uint8_t *)(static_cast<uint8_t *>(m_pStart) + 0x01); }

    /// @brief Gets the address of the handle field.
    uint16_t *getHandleAddr() const { return (uint16_t *)(static_cast<uint8_t *>(m_pStart) + 0x02); }

    /// @brief Gets the address of the socket designation field.
    uint8_t *getSocketDesignationAddr() const { return (uint8_t *)(static_cast<uint8_t *>(m_pStart) + 0x04); }

    /// @brief Gets the address of the processor type field.
    uint8_t *getProcessorTypeAddr() const { return (uint8_t *)(static_cast<uint8_t *>(m_pStart) + 0x05); }

    /// @brief Gets the address of the processor family field.
    uint8_t *getProcessorFamilyAddr() const { return (uint8_t *)(static_cast<uint8_t *>(m_pStart) + 0x06); }

    /// @brief Gets the address of the processor manufacturer field.
    uint8_t *getProcessorManufacturerAddr() const { return (uint8_t *)(static_cast<uint8_t *>(m_pStart) + 0x07); }

    /// @brief Gets the address of the processor ID field.
    uint32_t *getProcessorIDAddr() const { return (uint32_t *)(static_cast<uint8_t *>(m_pStart) + 0x08); }

    /// @brief Gets the address of the processor version field.
    uint8_t *getProcessorVersionAddr() const { return (uint8_t *)(static_cast<uint8_t *>(m_pStart) + 0x10); }

    /// @brief Gets the address of the voltage field.
    uint8_t *getVoltageAddr() const { return (uint8_t *)(static_cast<uint8_t *>(m_pStart) + 0x11); }

    /// @brief Gets the address of the external clock field.
    uint16_t *getExternalClockAddr() const { return (uint16_t *)(static_cast<uint8_t *>(m_pStart) + 0x12); }

    /// @brief Gets the address of the maximum speed field.
    uint16_t *getMaxSpeedAddr() const { return (uint16_t *)(static_cast<uint8_t *>(m_pStart) + 0x14); }

    /// @brief Gets the address of the current speed field.
    uint16_t *getCurrentSpeedAddr() const { return (uint16_t *)(static_cast<uint8_t *>(m_pStart) + 0x16); }

    /// @brief Gets the address of the status field.
    uint8_t *getStatusAddr() const { return (uint8_t *)(static_cast<uint8_t *>(m_pStart) + 0x18); }

    /// @brief Gets the address of the processor upgrade field.
    uint8_t *getProcessorUpgradeAddr() const { return (uint8_t *)(static_cast<uint8_t *>(m_pStart) + 0x19); }

    /// @brief Gets the address of the L1 cache handle field.
    uint16_t *getL1CacheHandleAddr() const { return (uint16_t *)(static_cast<uint8_t *>(m_pStart) + 0x1A); }

    /// @brief Gets the address of the L2 cache handle field.
    uint16_t *getL2CacheHandleAddr() const { return (uint16_t *)(static_cast<uint8_t *>(m_pStart) + 0x1C); }

    /// @brief Gets the address of the L3 cache handle field.
    uint16_t *getL3CacheHandleAddr() const { return (uint16_t *)(static_cast<uint8_t *>(m_pStart) + 0x1E); }

    /// @brief Gets the address of the serial number field.
    uint8_t *getSerialNumberAddr() const { return (uint8_t *)(static_cast<uint8_t *>(m_pStart) + 0x20); }

    /// @brief Gets the address of the asset tag number field.
    uint8_t *getAssetTagAddr() const { return (uint8_t *)(static_cast<uint8_t *>(m_pStart) + 0x21); }

    /// @brief Gets the address of the part number field.
    uint8_t *getPartNumberAddr() const { return (uint8_t *)(static_cast<uint8_t *>(m_pStart) + 0x22); }

    /// @brief Gets the address of the core count field.
    uint8_t *getCoreCountAddr() const { return (uint8_t *)(static_cast<uint8_t *>(m_pStart) + 0x23); }

    /// @brief Gets the address of the core enabled field.
    uint8_t *getCoreEnabledAddr() const { return (uint8_t *)(static_cast<uint8_t *>(m_pStart) + 0x24); }

    /// @brief Gets the address of the thread count field.
    uint8_t *getThreadCountAddr() const { return (uint8_t *)(static_cast<uint8_t *>(m_pStart) + 0x25); }

    /// @brief Gets the address of the processor characteristics field.
    uint16_t *getProcessorCharacteristicsAddr() const { return (uint16_t *)(static_cast<uint8_t *>(m_pStart) + 0x26); }

    /// @brief Gets the address of the secondary processor family field.
    uint16_t *getProcessorFamily2Addr() const { return (uint16_t *)(static_cast<uint8_t *>(m_pStart) + 0x28); }
};
#endif /* E783A862_53F3_4C99_B779_8123301B75AE */
