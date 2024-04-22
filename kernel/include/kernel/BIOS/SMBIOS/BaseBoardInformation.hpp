/**
 * @file BaseBoardInformation.hpp
 * @author Oguzhan KAYNAK
 * @brief Reading the Baseboard (or Module) Information (Type 2) field in SMBIOS
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
#ifndef F26356CA_FE94_49CC_9832_A5D11CC1F3C2
#define F26356CA_FE94_49CC_9832_A5D11CC1F3C2

#include <stdint.h>
#include <stdio.h>
#include "kernel/BIOS/BIOSStructureCommon.hpp"

class BaseBoardInformation : public BIOSStructureCommon
{
public:
    BaseBoardInformation() = default;

    /**
     * @brief Constructor with parameter to initialize the BaseBoardInformation object.
     * @param p_start Pointer to the start address of the BaseBoardInformation structure.
     */
    BaseBoardInformation(void *p_start)
    {
        Scan(p_start);
    }

    /**
     * @brief Scans the BaseBoardInformation structure and extracts relevant information.
     * @param p_start Pointer to the start address of the BaseBoardInformation structure.
     * @return Pointer to the end address of the scanned structure.
     */
    void *Scan(void *p_start) override
    {
        m_pStart = p_start;

        int end = 1;
        uint8_t *ptr = (uint8_t *)m_pStart + getLength();

        for (; ptr[end - 1] != '\0' || ptr[end] != '\0'; end++);

        m_pEnd = ptr + end + 1;

        extractStringAddress((const char *)ptr, (const char *)ptr + end, m_strings, StringCount);

        return m_pEnd;
    }

    // Getter functions
    /**
     * @brief Gets the type of the structure.
     * @return The type of the structure.
     */
    uint8_t getType() const { return *getTypeAddr(); }

    /**
     * @brief Gets the length of the structure.
     * @return The length of the structure.
     */
    uint8_t getLength() const { return *getLengthAddr(); }

    /**
     * @brief Gets the handle of the structure.
     * @return The handle of the structure.
     */
    uint16_t getHandle() const { return *getHandleAddr(); }

    /**
     * @brief Gets the manufacturer of the baseboard.
     * @return The manufacturer of the baseboard.
     */
    uint8_t getManufacturer() const { return *getManufacturerAddr(); }

    /**
     * @brief Gets the product name of the baseboard.
     * @return The product name of the baseboard.
     */
    uint8_t getProduct() const { return *getProductAddr(); }

    /**
     * @brief Gets the version of the baseboard.
     * @return The version of the baseboard.
     */
    uint8_t getVersion() const { return *getVersionAddr(); }

    /**
     * @brief Gets the serial number of the baseboard.
     * @return The serial number of the baseboard.
     */
    uint8_t getSerialNumber() const { return *getSerialNumberAddr(); }

    /**
     * @brief Gets the asset tag of the baseboard.
     * @return The asset tag of the baseboard.
     */
    uint8_t getAssetTag() const { return *getAssetTagAddr(); }

    /**
     * @brief Gets the feature flags of the baseboard.
     * @return The feature flags of the baseboard.
     */
    uint8_t getFeatureFlags() const { return *getFeatureFlagsAddr(); }

    /**
     * @brief Gets the location in chassis of the baseboard.
     * @return The location in chassis of the baseboard.
     */
    uint8_t getLocationInChassis() const { return *getLocationInChassisAddr(); }

    /**
     * @brief Gets the chassis handle of the baseboard.
     * @return The chassis handle of the baseboard.
     */
    uint16_t getChassisHandle() const { return *getChassisHandleAddr(); }

    /**
     * @brief Gets the board type of the baseboard.
     * @return The board type of the baseboard.
     */
    uint8_t getBoardType() const { return *getBoardTypeAddr(); }

    /**
     * @brief Gets the number of contained object handles.
     * @return The number of contained object handles.
     */
    uint8_t getNumContainedObjectHandles() const { return *getNumContainedObjectHandlesAddr(); }

    /**
     * @brief Gets the contained object handles.
     * @return The contained object handles.
     */
    uint16_t *getContainedObjectHandles() const { return getContainedObjectHandlesAddr(); }

    /// @brief Prints the extracted information of the Baseboard structure.
    void print() const override
    {
        printf("Type: %u\n", getType());
        printf("Length: %u\n", getLength());
        printf("Handle: %u\n", getHandle());
        printf("Manufacturer: %u\n", getManufacturer());
        printf("Product: %u\n", getProduct());
        printf("Version: %u\n", getVersion());
        printf("Serial Number: %u\n", getSerialNumber());
        printf("Asset Tag: %u\n", getAssetTag());
        printf("Feature Flags: %u\n", getFeatureFlags());
        printf("Location in Chassis: %u\n", getLocationInChassis());
        printf("Chassis Handle: %u\n", getChassisHandle());
        printf("Board Type: %u\n", getBoardType());
        printf("Number of Contained Object Handles: %u\n", getNumContainedObjectHandles());
        printf("Contained Object Handles: %u", getContainedObjectHandles());
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
        ManufacturerString;
        ProductNameString;
        VersionString;
        SerialNumberString;
        AssetTAGString;
        LocationinChassisString;
    */
    static constexpr uint8_t StringCount = 6; ///< Number of string fields.
    const char *m_strings[StringCount]{};    ///< Array to store string fields.

    // Private methods to find addresses
    /**
     * @brief Gets the address of the type field in the BaseBoardInformation structure.
     * @return The address of the type field.
     */
    uint8_t *getTypeAddr() const { return (uint8_t *)(static_cast<uint8_t *>(m_pStart) + 0x00); }

    /**
     * @brief Gets the address of the length field in the BaseBoardInformation structure.
     * @return The address of the length field.
     */
    uint8_t *getLengthAddr() const { return (uint8_t *)(static_cast<uint8_t *>(m_pStart) + 0x01); }

    /**
     * @brief Gets the address of the handle field in the BaseBoardInformation structure.
     * @return The address of the handle field.
     */
    uint16_t *getHandleAddr() const { return (uint16_t *)(static_cast<uint8_t *>(m_pStart) + 0x02); }

    /**
     * @brief Gets the address of the manufacturer field in the BaseBoardInformation structure.
     * @return The address of the manufacturer field.
     */
    uint8_t *getManufacturerAddr() const { return (uint8_t *)(static_cast<uint8_t *>(m_pStart) + 0x04); }

    /**
     * @brief Gets the address of the product field in the BaseBoardInformation structure.
     * @return The address of the product field.
     */
    uint8_t *getProductAddr() const { return (uint8_t *)(static_cast<uint8_t *>(m_pStart) + 0x05); }

    /**
     * @brief Gets the address of the version field in the BaseBoardInformation structure.
     * @return The address of the version field.
     */
    uint8_t *getVersionAddr() const { return (uint8_t *)(static_cast<uint8_t *>(m_pStart) + 0x06); }

    /**
     * @brief Gets the address of the serial number field in the BaseBoardInformation structure.
     * @return The address of the serial number field.
     */
    uint8_t *getSerialNumberAddr() const { return (uint8_t *)(static_cast<uint8_t *>(m_pStart) + 0x07); }

    /**
     * @brief Gets the address of the asset tag field in the BaseBoardInformation structure.
     * @return The address of the asset tag field.
     */
    uint8_t *getAssetTagAddr() const { return (uint8_t *)(static_cast<uint8_t *>(m_pStart) + 0x08); }

    /**
     * @brief Gets the address of the feature flags field in the BaseBoardInformation structure.
     * @return The address of the feature flags field.
     */
    uint8_t *getFeatureFlagsAddr() const { return (uint8_t *)(static_cast<uint8_t *>(m_pStart) + 0x09); }

    /**
     * @brief Gets the address of the location in chassis field in the BaseBoardInformation structure.
     * @return The address of the location in chassis field.
     */
    uint8_t *getLocationInChassisAddr() const { return (uint8_t *)(static_cast<uint8_t *>(m_pStart) + 0x0A); }
    
    /**
     * @brief Gets the address of the chassis handle field in the BaseBoardInformation structure.
     * @return The address of the chassis handle field.
     */
    uint16_t *getChassisHandleAddr() const { return (uint16_t *)(static_cast<uint8_t *>(m_pStart) + 0x0B); }

    /**
     * @brief Gets the address of the board type field in the BaseBoardInformation structure.
     * @return The address of the board type field.
     */
    uint8_t *getBoardTypeAddr() const { return (uint8_t *)(static_cast<uint8_t *>(m_pStart) + 0x0D); }

    /**
     * @brief Gets the address of the number of contained object handles field in the BaseBoardInformation structure.
     * @return The address of the number of contained object handles field.
     */
    uint8_t *getNumContainedObjectHandlesAddr() const { return (uint8_t *)(static_cast<uint8_t *>(m_pStart) + 0x0E); }

    /**
     * @brief Gets the address of the contained object handles field in the BaseBoardInformation structure.
     * @return The address of the contained object handles field.
     */
    uint16_t *getContainedObjectHandlesAddr() const { return (uint16_t *)(static_cast<uint8_t *>(m_pStart) + 0x0F); }
};

#endif /* F26356CA_FE94_49CC_9832_A5D11CC1F3C2 */
