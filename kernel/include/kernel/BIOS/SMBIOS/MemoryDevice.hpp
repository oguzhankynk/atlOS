/**
 * @file MemoryDevice.hpp
 * @author Oguzhan KAYNAK
 * @brief Reading the Memory Array Mapped Address (Type 19) structure field in SMBIOS
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
#ifndef CADF833A_320D_4F60_9ABA_A209CDA30F4C
#define CADF833A_320D_4F60_9ABA_A209CDA30F4C

#include <stdint.h>
#include <stdio.h>
#include "kernel/BIOS/BIOSStructureCommon.hpp"

/// @brief Represents a Memory Device structure.
class MemoryDevice : public BIOSStructureCommon
{
public:
    /// @brief Default constructor.
    MemoryDevice() = default;

    /// @brief Constructor that initializes the Memory Device structure by scanning the given address.
    /// @param p_start Start address of the Memory Device structure.
    MemoryDevice(void *p_start)
    {
        Scan(p_start);
    }

    /// @brief Scans the Memory Device structure starting from the given address.
    /// @param p_start Start address of the Memory Device structure.
    /// @return End address of the scanned Memory Device structure.
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

    /// @brief Gets the type of the Memory Device.
    /// @return Type of the Memory Device.
    uint8_t getType() const { return *getTypeAddr(); }

    /// @brief Gets the length of the Memory Device structure.
    /// @return Length of the Memory Device structure.
    uint8_t getLength() const { return *getLengthAddr(); }

    /// @brief Gets the handle associated with the Memory Device.
    /// @return Handle associated with the Memory Device.
    uint16_t getHandle() const { return *getHandleAddr(); }

    /// @brief Gets the handle of the Physical Memory Array associated with the Memory Device.
    /// @return Handle of the Physical Memory Array associated with the Memory Device.
    uint16_t getPhysicalMemoryArrayHandle() const { return *getPhysicalMemoryArrayHandleAddr(); }

    /// @brief Gets the handle of the Memory Error Information associated with the Memory Device.
    /// @return Handle of the Memory Error Information associated with the Memory Device.
    uint16_t getMemoryErrorInformationHandle() const { return *getMemoryErrorInformationHandleAddr(); }

    /// @brief Gets the total width of the Memory Device.
    /// @return Total width of the Memory Device.
    uint16_t getTotalWidth() const { return *getTotalWidthAddr(); }

    /// @brief Gets the data width of the Memory Device.
    /// @return Data width of the Memory Device.
    uint16_t getDataWidth() const { return *getDataWidthAddr(); }

    /// @brief Gets the size of the Memory Device.
    /// @return Size of the Memory Device.
    uint16_t getSize() const { return *getSizeAddr(); }

    /// @brief Gets the form factor of the Memory Device.
    /// @return Form factor of the Memory Device.
    uint8_t getFormFactor() const { return *getFormFactorAddr(); }

    /// @brief Gets the device set of the Memory Device.
    /// @return Device set of the Memory Device.
    uint8_t getDeviceSet() const { return *getDeviceSetAddr(); }

    /// @brief Gets the device locator of the Memory Device.
    /// @return Device locator of the Memory Device.
    uint8_t getDeviceLocator() const { return *getDeviceLocatorAddr(); }

    /// @brief Gets the bank locator of the Memory Device.
    /// @return Bank locator of the Memory Device.
    uint8_t getBankLocator() const { return *getBankLocatorAddr(); }

    /// @brief Gets the memory type of the Memory Device.
    /// @return Memory type of the Memory Device.
    uint8_t getMemoryType() const { return *getMemoryTypeAddr(); }

    /// @brief Gets the type detail of the Memory Device.
    /// @return Type detail of the Memory Device.
    uint16_t getTypeDetail() const { return *getTypeDetailAddr(); }

    /// @brief Gets the speed of the Memory Device.
    /// @return Speed of the Memory Device.
    uint16_t getSpeed() const { return *getSpeedAddr(); }

    /// @brief Gets the manufacturer of the Memory Device.
    /// @return Manufacturer of the Memory Device.
    uint8_t getManufacturer() const { return *getManufacturerAddr(); }

    /// @brief Gets the serial number of the Memory Device.
    /// @return Serial number of the Memory Device.
    uint8_t getSerialNumber() const { return *getSerialNumberAddr(); }

    /// @brief Gets the asset tag of the Memory Device.
    /// @return Asset tag of the Memory Device.
    uint8_t getAssetTag() const { return *getAssetTagAddr(); }

    /// @brief Gets the part number of the Memory Device.
    /// @return Part number of the Memory Device.
    uint8_t getPartNumber() const { return *getPartNumberAddr(); }

    /// @brief Gets the attribute of the Memory Device.
    /// @return Attribute of the Memory Device.
    uint8_t getAttribute() const { return *getAttributeAddr(); }

    /// @brief Gets the extended size of the Memory Device.
    /// @return Extended size of the Memory Device.
    uint32_t getExtendedSize() const { return *getExtendedSizeAddr(); }

    /// @brief Gets the configured memory clock speed of the Memory Device.
    /// @return Configured memory clock speed of the Memory Device.
    uint16_t getConfiguredMemoryClockSpeed() const { return *getConfiguredMemoryClockSpeedAddr(); }

    /// @brief Gets the minimum voltage of the Memory Device.
    /// @return Minimum voltage of the Memory Device.
    uint16_t getMinimumVoltage() const { return *getMinimumVoltageAddr(); }

    /// @brief Gets the maximum voltage of the Memory Device.
    /// @return Maximum voltage of the Memory Device.
    uint16_t getMaximumVoltage() const { return *getMaximumVoltageAddr(); }

    /// @brief Gets the configured voltage of the Memory Device.
    /// @return Configured voltage of the Memory Device.
    uint16_t getConfiguredVoltage() const { return *getConfiguredVoltageAddr(); }

    /// @brief Prints all the values of the Memory Device table.
    void print() const override
    {
        // Print each field with its value
        printf("Type: %u\n", *getTypeAddr());
        printf("Length: %u\n", *getLengthAddr());
        printf("Handle: %u\n", *getHandleAddr());
        printf("Physical Memory Array Handle: %u\n", *getPhysicalMemoryArrayHandleAddr());
        printf("Memory Error Information Handle: %u\n", *getMemoryErrorInformationHandleAddr());
        printf("Total Width: %u\n", *getTotalWidthAddr());
        printf("Data Width: %u\n", *getDataWidthAddr());
        printf("Size: %u\n", *getSizeAddr());
        printf("Form Factor: %u\n", *getFormFactorAddr());
        printf("Device Set: %u\n", *getDeviceSetAddr());
        printf("Device Locator: %u\n", getDeviceLocatorAddr());
        printf("Bank Locator: %u\n", getBankLocatorAddr());
        printf("Memory Type: %u\n", *getMemoryTypeAddr());
        printf("Type Detail: %u\n", *getTypeDetailAddr());
        printf("Speed: %u\n", *getSpeedAddr());
        printf("Manufacturer: %u\n", getManufacturerAddr());
        printf("Serial Number: %u\n", getSerialNumberAddr());
        printf("Asset Tag: %u\n", getAssetTagAddr());
        printf("Part Number: %u\n", getPartNumberAddr());
        printf("Attribute: %u\n", *getAttributeAddr());
        printf("Extended Size: %u\n", *getExtendedSizeAddr());
        printf("Configured Memory Clock Speed: %u\n", *getConfiguredMemoryClockSpeedAddr());
        printf("Minimum Voltage: %u Maximum Voltage: %u Configured Voltage: %u \n",
               *getMinimumVoltageAddr(), *getMaximumVoltageAddr(), *getConfiguredVoltageAddr());

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
        Device Locator;
        Bank Locator;
        Manufacturer;
        Serial Number;
        Asset Tag;
        Part Number;
    */
    static constexpr uint8_t StringCount = 6; ///< Number of string fields.
    const char *m_strings[StringCount]{};     ///< Array to store string fields.

    // Private methods to get the addresses of different fields.

    /// @brief Gets the address of the Type field.
    /// @return Pointer to the Type field.
    uint8_t *getTypeAddr() const { return reinterpret_cast<uint8_t *>(reinterpret_cast<uint8_t *>(m_pStart) + 0x00); }

    /// @brief Gets the address of the Length field.
    /// @return Pointer to the Length field.
    uint8_t *getLengthAddr() const { return reinterpret_cast<uint8_t *>(reinterpret_cast<uint8_t *>(m_pStart) + 0x01); }

    /// @brief Gets the address of the Handle field.
    /// @return Pointer to the Handle field.
    uint16_t *getHandleAddr() const { return reinterpret_cast<uint16_t *>(reinterpret_cast<uint8_t *>(m_pStart) + 0x02); }

    /// @brief Gets the address of the Physical Memory Array Handle field.
    /// @return Pointer to the Physical Memory Array Handle field.
    uint16_t *getPhysicalMemoryArrayHandleAddr() const { return reinterpret_cast<uint16_t *>(reinterpret_cast<uint8_t *>(m_pStart) + 0x04); }

    /// @brief Gets the address of the Memory Error Information Handle field.
    /// @return Pointer to the Memory Error Information Handle field.
    uint16_t *getMemoryErrorInformationHandleAddr() const { return reinterpret_cast<uint16_t *>(reinterpret_cast<uint8_t *>(m_pStart) + 0x06); }

    /// @brief Gets the address of the Total Width field.
    /// @return Pointer to the Total Width field.
    uint16_t *getTotalWidthAddr() const { return reinterpret_cast<uint16_t *>(reinterpret_cast<uint8_t *>(m_pStart) + 0x08); }

    /// @brief Gets the address of the Data Width field.
    /// @return Pointer to the Data Width field.
    uint16_t *getDataWidthAddr() const { return reinterpret_cast<uint16_t *>(reinterpret_cast<uint8_t *>(m_pStart) + 0x0A); }

    /// @brief Gets the address of the Size field.
    /// @return Pointer to the Size field.
    uint16_t *getSizeAddr() const { return reinterpret_cast<uint16_t *>(reinterpret_cast<uint8_t *>(m_pStart) + 0x0C); }

    /// @brief Gets the address of the Form Factor field.
    /// @return Pointer to the Form Factor field.
    uint8_t *getFormFactorAddr() const { return reinterpret_cast<uint8_t *>(reinterpret_cast<uint8_t *>(m_pStart) + 0x0E); }

    /// @brief Gets the address of the Device Set field.
    /// @return Pointer to the Device Set field.
    uint8_t *getDeviceSetAddr() const { return reinterpret_cast<uint8_t *>(reinterpret_cast<uint8_t *>(m_pStart) + 0x0F); }

    /// @brief Gets the address of the Device Locator field.
    /// @return Pointer to the Device Locator field.
    uint8_t *getDeviceLocatorAddr() const { return reinterpret_cast<uint8_t *>(reinterpret_cast<uint8_t *>(m_pStart) + 0x10); }

    /// @brief Gets the address of the Bank Locator field.
    /// @return Pointer to the Bank Locator field.
    uint8_t *getBankLocatorAddr() const { return reinterpret_cast<uint8_t *>(reinterpret_cast<uint8_t *>(m_pStart) + 0x11); }

    /// @brief Gets the address of the Memory Type field.
    /// @return Pointer to the Memory Type field.
    uint8_t *getMemoryTypeAddr() const { return reinterpret_cast<uint8_t *>(reinterpret_cast<uint8_t *>(m_pStart) + 0x12); }

    /// @brief Gets the address of the Type Detail field.
    /// @return Pointer to the Type Detail field.
    uint16_t *getTypeDetailAddr() const { return reinterpret_cast<uint16_t *>(reinterpret_cast<uint8_t *>(m_pStart) + 0x13); }

    /// @brief Gets the address of the Speed field.
    /// @return Pointer to the Speed field.
    uint16_t *getSpeedAddr() const { return reinterpret_cast<uint16_t *>(reinterpret_cast<uint8_t *>(m_pStart) + 0x15); }

    /// @brief Gets the address of the Manufacturer field.
    /// @return Pointer to the Manufacturer field.
    uint8_t *getManufacturerAddr() const { return reinterpret_cast<uint8_t *>(reinterpret_cast<uint8_t *>(m_pStart) + 0x17); }

    /// @brief Gets the address of the Serial Number field.
    /// @return Pointer to the Serial Number field.
    uint8_t *getSerialNumberAddr() const { return reinterpret_cast<uint8_t *>(reinterpret_cast<uint8_t *>(m_pStart) + 0x18); }

    /// @brief Gets the address of the Asset Tag field.
    /// @return Pointer to the Asset Tag field.
    uint8_t *getAssetTagAddr() const { return reinterpret_cast<uint8_t *>(reinterpret_cast<uint8_t *>(m_pStart) + 0x19); }

    /// @brief Gets the address of the Part Number field.
    /// @return Pointer to the Part Number field.
    uint8_t *getPartNumberAddr() const { return reinterpret_cast<uint8_t *>(reinterpret_cast<uint8_t *>(m_pStart) + 0x1A); }

    /// @brief Gets the address of the Attribute field.
    /// @return Pointer to the Attribute field.
    uint8_t *getAttributeAddr() const { return reinterpret_cast<uint8_t *>(reinterpret_cast<uint8_t *>(m_pStart) + 0x1B); }

    /// @brief Gets the address of the Extended Size field.
    /// @return Pointer to the Extended Size field.
    uint32_t *getExtendedSizeAddr() const { return reinterpret_cast<uint32_t *>(reinterpret_cast<uint8_t *>(m_pStart) + 0x1C); }

    /// @brief Gets the address of the Configured Memory Clock Speed field.
    /// @return Pointer to the Configured Memory Clock Speed field.
    uint16_t *getConfiguredMemoryClockSpeedAddr() const { return reinterpret_cast<uint16_t *>(reinterpret_cast<uint8_t *>(m_pStart) + 0x20); }

    /// @brief Gets the address of the Minimum Voltage field.
    /// @return Pointer to the Minimum Voltage field.
    uint16_t *getMinimumVoltageAddr() const { return reinterpret_cast<uint16_t *>(reinterpret_cast<uint8_t *>(m_pStart) + 0x22); }

    /// @brief Gets the address of the Maximum Voltage field.
    /// @return Pointer to the Maximum Voltage field.
    uint16_t *getMaximumVoltageAddr() const { return reinterpret_cast<uint16_t *>(reinterpret_cast<uint8_t *>(m_pStart) + 0x24); }

    /// @brief Gets the address of the Configured Voltage field.
    /// @return Pointer to the Configured Voltage field.
    uint16_t *getConfiguredVoltageAddr() const { return reinterpret_cast<uint16_t *>(reinterpret_cast<uint8_t *>(m_pStart) + 0x26); }
};

#endif /* CADF833A_320D_4F60_9ABA_A209CDA30F4C */
