/**
 * @file SystemInformation.hpp
 * @author okaynak ()
 * @brief Reading the System Information (Type 1) structure field in SMBIOS
 * @date 2024-03-31
 * 
 * @copyright Copyright (c) 2024
 * 
 */
#ifndef B753BF1E_92A3_45A6_ABED_F40B5BE6B059
#define B753BF1E_92A3_45A6_ABED_F40B5BE6B059

#include <stdint.h>
#include <stdio.h>
#include "kernel/private/BIOS/BIOSStructureCommon.hpp"

/// @brief Class for handling System Information (Type 1) structure in SMBIOS.
class SystemInformation: public BIOSStructureCommon
{
public:
    /// @brief Default constructor.
    SystemInformation() = default;

    /// @brief Constructor that takes a pointer to the start address of the structure.
    /// @param p_start Pointer to the start address of the structure.
    SystemInformation(void *p_start)
    {
        Scan(p_start);
    };

    /// @brief Scans the structure and extracts relevant information.
    /// @param p_start Pointer to the start address of the structure.
    /// @return Pointer to the end address of the structure.
    void *Scan(void *p_start)
    {
        m_pStart = p_start;

        int end = 1;
        uint8_t *ptr = (uint8_t *)m_pStart + getLength();

        for (; ptr[end - 1] != '\0' || ptr[end] != '\0'; end++);

        m_pEnd = ptr + end + 1;

        extractStringAddress((const char *)ptr, (const char *)ptr + end, m_strings, StringCount);

        return m_pEnd;
    }

    /// @brief Retrieves the start address of the structure.
    /// @return Pointer to the start address of the structure.
    void *getStartAddr()
    {
        return m_pStart;
    }

    /// @brief Retrieves the end address of the structure.
    /// @return Pointer to the end address of the structure.
    void *getEndAddr()
    {
        return m_pEnd;
    }

    /// @brief Retrieves the type of the structure.
    /// @return Type of the structure.
    uint8_t getType()
    {
        return *getTypeAddr();
    }

    /// @brief Retrieves the length of the structure.
    /// @return Length of the structure.
    uint8_t getLength()
    {
        return *getLengthAddr();
    }

    /// @brief Retrieves the handle of the structure.
    /// @return Handle of the structure.
    uint16_t getHandle()
    {
        return *getHandleAddr();
    }

    /// @brief Retrieves the manufacturer of the system.
    /// @return Manufacturer of the system.
    uint8_t getManufacturer()
    {
        return *getManufacturerAddr();
    }

    /// @brief Retrieves the product name of the system.
    /// @return Product name of the system.
    uint8_t getProductName()
    {
        return *getProductNameAddr();
    }

    /// @brief Retrieves the version of the system.
    /// @return Version of the system.
    uint8_t getVersion()
    {
        return *getVersionAddr();
    }

    /// @brief Retrieves the serial number of the system.
    /// @return Serial number of the system.
    uint8_t getSerialNumber()
    {
        return *getSerialNumberAddr();
    }

    /// @brief Retrieves the UUID of the system.
    /// @return Pointer to the UUID of the system.
    const uint8_t *getUuid()
    {
        return getUuidAddr();
    }

    /// @brief Retrieves the wake-up type of the system.
    /// @return Wake-up type of the system.
    uint8_t getWakeUpType()
    {
        return *getWakeUpTypeAddr();
    }

    /// @brief Retrieves the SKU number of the system.
    /// @return SKU number of the system.
    uint8_t getSkuNumber()
    {
        return *getSkuNumberAddr();
    }

    /// @brief Retrieves the family of the system.
    /// @return Family of the system.
    uint8_t getFamily()
    {
        return *getFamilyAddr();
    }

    /// @brief Prints the extracted information of the system structure.
    void print()
    {
        printf("Type: %u\n", getType());
        printf("Length: %u\n", getLength());
        printf("Handle: %u\n", getHandle());
        printf("Manufacturer: %u\n", getManufacturer());
        printf("Product Name: %u\n", getProductName());
        printf("Version: %u\n", getVersion());
        printf("Serial Number: %u\n", getSerialNumber());
        printf("UUID: ");
        for (size_t i = 0; i < 16; ++i)
        {
            printf("%X ", *(getUuid() + i) & 0xFF);
        }
        printf("\n");
        printf("Wake-up Type: %u\n", getWakeUpType());
        printf("SKU Number: %u\n", getSkuNumber());
        printf("Family: %u\n", getFamily());
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
        SKUNumberString;
        FamilyString;
    */
    static constexpr uint8_t StringCount = 6; ///< Number of string fields.
    const char *m_strings[StringCount]{};    ///< Array to store string fields.

    /// @brief Retrieves the address of the type field.
    /// @return Address of the type field.
    uint8_t *getTypeAddr()
    {
        return (uint8_t *)(static_cast<uint8_t *>(m_pStart) + 0);
    }

    /// @brief Retrieves the address of the length field.
    /// @return Address of the length field.
    uint8_t *getLengthAddr()
    {
        return (uint8_t *)(static_cast<uint8_t *>(m_pStart) + 1);
    }

    /// @brief Retrieves the address of the handle field.
    /// @return Address of the handle field.
    uint16_t *getHandleAddr()
    {
        return (uint16_t *)(static_cast<uint8_t *>(m_pStart) + 2);
    }

    /// @brief Retrieves the address of the manufacturer field.
    /// @return Address of the manufacturer field.
    uint8_t *getManufacturerAddr()
    {
        return (uint8_t *)(static_cast<uint8_t *>(m_pStart) + 4);
    }

    /// @brief Retrieves the address of the product name field.
    /// @return Address of the product name field.
    uint8_t *getProductNameAddr()
    {
        return (uint8_t *)(static_cast<uint8_t *>(m_pStart) + 5);
    }

    /// @brief Retrieves the address of the version field.
    /// @return Address of the version field.
    uint8_t *getVersionAddr()
    {
        return (uint8_t *)(static_cast<uint8_t *>(m_pStart) + 6);
    }

    /// @brief Retrieves the address of the serial number field.
    /// @return Address of the serial number field.
    uint8_t *getSerialNumberAddr()
    {
        return (uint8_t *)(static_cast<uint8_t *>(m_pStart) + 7);
    }

    /// @brief Retrieves the address of the UUID field.
    /// @return Address of the UUID field.
    uint8_t *getUuidAddr()
    {
        return (uint8_t *)(static_cast<uint8_t *>(m_pStart) + 8);
    }

    /// @brief Retrieves the address of the wake-up type field.
    /// @return Address of the wake-up type field.
    uint8_t *getWakeUpTypeAddr()
    {
        return (uint8_t *)(static_cast<uint8_t *>(m_pStart) + 24);
    }

    /// @brief Retrieves the address of the SKU number field.
    /// @return Address of the SKU number field.
    uint8_t *getSkuNumberAddr()
    {
        return (uint8_t *)(static_cast<uint8_t *>(m_pStart) + 25);
    }

    /// @brief Retrieves the address of the family field.
    /// @return Address of the family field.
    uint8_t *getFamilyAddr()
    {
        return (uint8_t *)(static_cast<uint8_t *>(m_pStart) + 26);
    }
};

#endif /* B753BF1E_92A3_45A6_ABED_F40B5BE6B059 */
