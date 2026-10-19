/**
 * @file system_enclosure_chassis.hpp
 * @author Oguzhan KAYNAK
 * @brief Reading the System Enclosure or Chassis (Type 3) structure field in SMBIOS
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
#ifndef D9D2E7CD_CE66_4545_AC83_AB1DD86799B2
#define D9D2E7CD_CE66_4545_AC83_AB1DD86799B2

#include <stdint.h>
#include <stdio.h>
#include <firmware/bios/structure_common.hpp>

/// @brief Class representing System Enclosure or Chassis structure, inheriting from BIOSStructureCommon.
class SystemEnclosureChassis : public BIOSStructureCommon
{
public:
    /**
     * @brief Default constructor for SystemEnclosureChassis class.
     */
    SystemEnclosureChassis() = default;

    /**
     * @brief Constructor for SystemEnclosureChassis class.
     * @param p_start Pointer to the start of the structure.
     */
    SystemEnclosureChassis(void *p_start)
    {
        Scan(p_start);
    }

    /**
     * @brief Scan function to read the System Enclosure or Chassis Information structure.
     * @param p_start Pointer to the start of the structure.
     * @return Pointer to the end address of the SystemEnclosureChassis
     */
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

    // Getter functions
    /**
     * @brief Get the type of the structure.
     * @return Type of the structure.
     */
    uint8_t getType() const { return *getTypeAddr(); }

    /**
     * @brief Get the length of the structure.
     * @return Length of the structure.
     */
    uint8_t getLength() const { return *getLengthAddr(); }

    /**
     * @brief Get the handle of the structure.
     * @return Handle of the structure.
     */
    uint16_t getHandle() const { return *getHandleAddr(); }

    /**
     * @brief Get the manufacturer of the enclosure or chassis.
     * @return Manufacturer of the enclosure or chassis.
     */
    uint8_t getManufacturer() const { return *getManufacturerAddr(); }

    /**
     * @brief Get the type of the enclosure or chassis.
     * @return Type of the enclosure or chassis.
     */
    uint8_t getType2() const { return *getType2Addr(); }

    /**
     * @brief Get the version of the enclosure or chassis.
     * @return Version of the enclosure or chassis.
     */
    uint8_t getVersion() const { return *getVersionAddr(); }

    /**
     * @brief Get the serial number of the enclosure or chassis.
     * @return Serial number of the enclosure or chassis.
     */
    uint8_t getSerialNumber() const { return *getSerialNumberAddr(); }

    /**
     * @brief Get the asset tag number of the enclosure or chassis.
     * @return Asset tag number of the enclosure or chassis.
     */
    uint8_t getAssetTagNumber() const { return *getAssetTagNumberAddr(); }

    /**
     * @brief Get the boot-up state of the enclosure or chassis.
     * @return Boot-up state of the enclosure or chassis.
     */
    uint8_t getBootUpState() const { return *getBootUpStateAddr(); }

    /**
     * @brief Get the power supply state of the enclosure or chassis.
     * @return Power supply state of the enclosure or chassis.
     */
    uint8_t getPowerSupplyState() const { return *getPowerSupplyStateAddr(); }

    /**
     * @brief Get the thermal state of the enclosure or chassis.
     * @return Thermal state of the enclosure or chassis.
     */
    uint8_t getThermalState() const { return *getThermalStateAddr(); }

    /**
     * @brief Get the security status of the enclosure or chassis.
     * @return Security status of the enclosure or chassis.
     */
    uint8_t getSecurityStatus() const { return *getSecurityStatusAddr(); }

    /**
     * @brief Get the OEM-defined information of the enclosure or chassis.
     * @return OEM-defined information of the enclosure or chassis.
     */
    uint32_t getOemDefined() const { return *getOemDefinedAddr(); }

    /**
     * @brief Get the height of the enclosure.
     * @return Height of the enclosure.
     */
    uint8_t getHeight() const { return *getHeightAddr(); }

    /**
     * @brief Get the number of power cords associated with the enclosure or chassis.
     * @return Number of power cords associated with the enclosure or chassis.
     */
    uint8_t getNumPowerCords() const { return *getNumPowerCordsAddr(); }

    /**
     * @brief Get the number of contained element records.
     * @return Number of contained element records.
     */
    uint8_t getContainedElementCount() const { return *getContainedElementCountAddr(); }

    /**
     * @brief Get the byte length of each contained element record.
     * @return Byte length of each contained element record.
     */
    uint8_t getContainedElementRecordLength() const { return *getContainedElementRecordLengthAddr(); }

    /**
     * @brief Get the contained elements of the enclosure or chassis.
     * @return Contained elements of the enclosure or chassis.
     */
    uint8_t *getContainedElements() const { return getContainedElementsAddr(); }

    /**
     * @brief Get the SKU number of the enclosure or chassis.
     * @return SKU number of the enclosure or chassis.
     */
    uint8_t getSkuNumber() const { return *getSkuNumberAddr(); }

    // Print function to display all values
    /**
     * @brief Print function to display all values of the System Enclosure or Chassis Information structure.
     */
    void print() const override
    {
        printf("Type: %u\n", getType());
        printf("Length: %u\n", getLength());
        printf("Handle: %u\n", getHandle());
        printf("Manufacturer: %u\n", getManufacturer());
        printf("Type: %u\n", getType2());
        printf("Version: %u\n", getVersion());
        printf("Serial Number: %u\n", getSerialNumber());
        printf("Asset Tag Number: %u\n", getAssetTagNumber());
        printf("Boot-up State: %u\n", getBootUpState());
        printf("Power Supply State: %u\n", getPowerSupplyState());
        printf("Thermal State: %u\n", getThermalState());
        printf("Security Status: %u\n", getSecurityStatus());
        printf("OEM-defined: %u\n", getOemDefined());
        printf("Height: %u\n", getHeight());
        printf("Number of Power Cords: %u\n", getNumPowerCords());
        printf("Contained Element Count: %u Contained Element Record Length: %u\n", getContainedElementCount(), getContainedElementRecordLength());
        printf("Contained Elements: ");
        uint8_t numElements = getContainedElementCount();
        uint8_t recordLength = getContainedElementRecordLength();
        for (int i = 0; i < numElements * recordLength; i++)
        {
            printf("%u ", getContainedElements()[i] & 0xFF);
        }
        printf("\n");
        printf("SKU Number: %u\n", getSkuNumber());
        for (size_t i = 0; i < StringCount; ++i)
        {
            printf("[%d]:%s ", i, m_strings[i]);
        }
        printf("\n");
        printf("Bitti");
    }

private:
    void *m_pStart; ///< Pointer to the start address of the BIOS structure.
    void *m_pEnd;   ///< Pointer to the end address of the BIOS structure.

    /*
       ManufacturerString;
       VersionString;
       SerialNumberString;
       AssetTagNumberString;
       SKUNumberString;
   */
    static constexpr uint8_t StringCount = 5; ///< Maximum number of strings to extract.
    const char *m_strings[StringCount]{};     ///< Array to store extracted strings.

    /// @brief Get the address of the Type field in the System Enclosure or Chassis Information structure.
    uint8_t *getTypeAddr() const { return (uint8_t *)(static_cast<uint8_t *>(m_pStart) + 0x00); }

    /// @brief Get the address of the Length field in the System Enclosure or Chassis Information structure.
    uint8_t *getLengthAddr() const { return (uint8_t *)(static_cast<uint8_t *>(m_pStart) + 0x01); }

    /// @brief Get the address of the Handle field in the System Enclosure or Chassis Information structure.
    uint16_t *getHandleAddr() const { return (uint16_t *)(static_cast<uint8_t *>(m_pStart) + 0x02); }

    /// @brief Get the address of the Manufacturer field in the System Enclosure or Chassis Information structure.
    uint8_t *getManufacturerAddr() const { return (uint8_t *)(static_cast<uint8_t *>(m_pStart) + 0x04); }

    /// @brief Get the address of the Type field (again) in the System Enclosure or Chassis Information structure.
    uint8_t *getType2Addr() const { return (uint8_t *)(static_cast<uint8_t *>(m_pStart) + 0x05); }

    /// @brief Get the address of the Version field in the System Enclosure or Chassis Information structure.
    uint8_t *getVersionAddr() const { return (uint8_t *)(static_cast<uint8_t *>(m_pStart) + 0x06); }

    /// @brief Get the address of the Serial Number field in the System Enclosure or Chassis Information structure.
    uint8_t *getSerialNumberAddr() const { return (uint8_t *)(static_cast<uint8_t *>(m_pStart) + 0x07); }

    /// @brief Get the address of the Asset Tag Number field in the System Enclosure or Chassis Information structure.
    uint8_t *getAssetTagNumberAddr() const { return (uint8_t *)(static_cast<uint8_t *>(m_pStart) + 0x08); }

    /// @brief Get the address of the Boot-up State field in the System Enclosure or Chassis Information structure.
    uint8_t *getBootUpStateAddr() const { return (uint8_t *)(static_cast<uint8_t *>(m_pStart) + 0x09); }

    /// @brief Get the address of the Power Supply State field in the System Enclosure or Chassis Information structure.
    uint8_t *getPowerSupplyStateAddr() const { return (uint8_t *)(static_cast<uint8_t *>(m_pStart) + 0x0A); }

    /// @brief Get the address of the Thermal State field in the System Enclosure or Chassis Information structure.
    uint8_t *getThermalStateAddr() const { return (uint8_t *)(static_cast<uint8_t *>(m_pStart) + 0x0B); }

    /// @brief Get the address of the Security Status field in the System Enclosure or Chassis Information structure.
    uint8_t *getSecurityStatusAddr() const { return (uint8_t *)(static_cast<uint8_t *>(m_pStart) + 0x0C); }

    /// @brief Get the address of the OEM-defined field in the System Enclosure or Chassis Information structure.
    uint32_t *getOemDefinedAddr() const { return (uint32_t *)(static_cast<uint8_t *>(m_pStart) + 0x0D); }

    /// @brief Get the address of the Height field in the System Enclosure or Chassis Information structure.
    uint8_t *getHeightAddr() const { return (uint8_t *)(static_cast<uint8_t *>(m_pStart) + 0x11); }

    /// @brief Get the address of the Number of Power Cords field in the System Enclosure or Chassis Information structure.
    uint8_t *getNumPowerCordsAddr() const { return (uint8_t *)(static_cast<uint8_t *>(m_pStart) + 0x12); }

    /// @brief Get the address of the Contained Element Count field in the System Enclosure or Chassis Information structure.
    uint8_t *getContainedElementCountAddr() const { return (uint8_t *)(static_cast<uint8_t *>(m_pStart) + 0x13); }

    /// @brief Get the address of the Contained Element Record Length field in the System Enclosure or Chassis Information structure.
    uint8_t *getContainedElementRecordLengthAddr() const { return (uint8_t *)(static_cast<uint8_t *>(m_pStart) + 0x14); }

    /// @brief Get the address of the Contained Elements field in the System Enclosure or Chassis Information structure.
    uint8_t *getContainedElementsAddr() const { return (uint8_t *)(static_cast<uint8_t *>(m_pStart) + 0x15); }

    /// @brief Get the address of the SKU Number field in the System Enclosure or Chassis Information structure.
    uint8_t *getSkuNumberAddr() const { return (uint8_t *)(static_cast<uint8_t *>(m_pStart) + 0x15 + getContainedElementCount() * getContainedElementRecordLength()); }
};
#endif /* D9D2E7CD_CE66_4545_AC83_AB1DD86799B2 */
