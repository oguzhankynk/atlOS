/**
 * @file BIOSInformation.hpp
 * @author okaynak ()
 * @brief Reading the BIOS Information (Type 0) structure field in SMBIOS
 * @date 2024-03-31
 * 
 * @copyright Copyright (c) 2024
 * 
 */
#ifndef AB42FE93_A33B_4AA5_9CF2_CBC263FCA5AE
#define AB42FE93_A33B_4AA5_9CF2_CBC263FCA5AE

#include <stdint.h>
#include <stdio.h>
#include "kernel/private/BIOS/BIOSStructureCommon.hpp"

/// @brief Class representing BIOS Information structure, inheriting from BIOSStructureCommon.
class BIOSInformation : public BIOSStructureCommon
{
public:
    /// @brief Default constructor.
    BIOSInformation() = default;

    /// @brief Constructor that initializes the object and scans the BIOS information.
    /// @param p_start Pointer to the start address of the BIOS structure.
    BIOSInformation(void *p_start)
    {
        Scan(p_start);
    };

    /// @brief Scans the BIOS information structure.
    /// @param p_start Pointer to the start address of the BIOS structure.
    /// @return Pointer to the end address of the BIOS structure.
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

    /// @brief Returns the start address of the BIOS structure.
    /// @return Start address of the BIOS structure.
    void *getStartAddr() const
    {
        return m_pStart;
    }

    /// @brief Returns the end address of the BIOS structure.
    /// @return End address of the BIOS structure.
    void *getEndAddr() const
    {
        return m_pEnd;
    }

    /// @brief Returns the type field of the BIOS structure.
    /// @return Type field of the BIOS structure.
    uint8_t getType() const
    {
        return *getTypeAddr();
    }

    /// @brief Returns the length field of the BIOS structure.
    /// @return Length field of the BIOS structure.
    uint8_t getLength() const
    {
        return *getLengthAddr();
    }

    /// @brief Returns the handle field of the BIOS structure.
    /// @return Handle field of the BIOS structure.
    uint16_t getHandle() const 
    {
        return *getHandleAddr();
    }

    /// @brief Returns the vendor string field of the BIOS structure.
    /// @return Vendor string field of the BIOS structure.
    uint8_t getVendorString() const
    {
        return *getVendorStringAddr();
    }

    /// @brief Returns the version string field of the BIOS structure.
    /// @return Version string field of the BIOS structure.
    uint8_t getVersionString() const
    {
        return *getVersionStringAddr();
    }

    /// @brief Returns the start segment field of the BIOS structure.
    /// @return Start segment field of the BIOS structure.
    uint16_t getStartSegment() const
    {
        return *getStartSegmentAddr();
    }

    /// @brief Returns the release date string field of the BIOS structure.
    /// @return Release date string field of the BIOS structure.
    uint8_t getReleaseDateString() const
    {
        return *getReleaseDateStringAddr();
    }

    /// @brief Returns the ROM size field of the BIOS structure.
    /// @return ROM size field of the BIOS structure.
    uint8_t getRomSize() const 
    {
        return *getRomSizeAddr();
    }

    /// @brief Returns the characteristics field of the BIOS structure.
    /// @return Characteristics field of the BIOS structure.
    uint64_t getCharacteristics() const
    {
        return *getCharacteristicsAddr();
    }

    /// @brief Returns the system BIOS major release field of the BIOS structure.
    /// @return System BIOS major release field of the BIOS structure.
    uint8_t getSystemBiosMajorRelease() const 
    {
        return *getSystemBiosMajorReleaseAddr();
    }

    /// @brief Returns the system BIOS minor release field of the BIOS structure.
    /// @return System BIOS minor release field of the BIOS structure.
    uint8_t getSystemBiosMinorRelease() const
    {
        return *getSystemBiosMinorReleaseAddr();
    }

    /// @brief Returns the embedded controller firmware major release field of the BIOS structure.
    /// @return Embedded controller firmware major release field of the BIOS structure.
    uint8_t getEmbeddedControllerFirmwareMajorRelease() const 
    {
        return *getEmbeddedControllerFirmwareMajorReleaseAddr();
    }

    /// @brief Returns the embedded controller firmware minor release field of the BIOS structure.
    /// @return Embedded controller firmware minor release field of the BIOS structure.
    uint8_t getEmbeddedControllerFirmwareMinorRelease() const 
    {
        return *getEmbeddedControllerFirmwareMinorReleaseAddr();
    }

    /// @brief Prints the BIOS structure information.
    void print() const override
    {
        printf("Type: %u\n", getType());
        printf("Length: %u\n", getLength());
        printf("Handle: %u\n", getHandle());
        printf("Vendor String: %u\n", getVendorString());
        printf("Version String: %u\n", getVersionString());
        printf("Start Segment: %u\n", getStartSegment());
        printf("Release Date: %u\n", getReleaseDateString());
        printf("ROM Size: %u\n", getRomSize());
        printf("Characteristics: %llu\n", (unsigned long long)getCharacteristics());
        //printf("Extension Byte 1: %u\n", );
        printf("System BIOS Major Release: %u\n", getSystemBiosMajorRelease());
        printf("System BIOS Minor Release: %u\n", getSystemBiosMinorRelease());
        printf("Embedded Controller Firmware Major Release: %u\n", getEmbeddedControllerFirmwareMajorRelease());
        printf("Embedded Controller Firmware Minor Release: %u\n", getEmbeddedControllerFirmwareMinorRelease());
        for (size_t i = 0; i < StringCount; ++i)
        {
            printf("[%d]:%s ", i, m_strings[i]);
        }
        printf("\n");
    }

private:
    void *m_pStart; ///< Pointer to the start address of the BIOS structure.
    void *m_pEnd;   ///< Pointer to the end address of the BIOS structure.

    /*
       vendorString;
       versionString;
       releaseDateString;
   */
    static constexpr uint8_t StringCount = 3; ///< Maximum number of strings to extract.
    const char *m_strings[StringCount]{};     ///< Array to store extracted strings.

    /// @brief Returns the address of the type field in the BIOS structure.
    uint8_t *getTypeAddr() const
    {
        return (uint8_t *)(static_cast<uint8_t *>(m_pStart) + 0);
    }

    /// @brief Returns the address of the length field in the BIOS structure.
    uint8_t *getLengthAddr() const
    {
        return (uint8_t *)(static_cast<uint8_t *>(m_pStart) + 1);
    }

    /// @brief Returns the address of the handle field in the BIOS structure.
    uint16_t *getHandleAddr() const
    {
        return (uint16_t *)(static_cast<uint8_t *>(m_pStart) + 2);
    }

    /// @brief Returns the address of the vendor string field in the BIOS structure.
    uint8_t *getVendorStringAddr() const 
    {
        return (uint8_t *)(static_cast<uint8_t *>(m_pStart) + 4);
    }

    /// @brief Returns the address of the version string field in the BIOS structure.
    uint8_t *getVersionStringAddr() const
    {
        return (uint8_t *)(static_cast<uint8_t *>(m_pStart) + 5);
    }

    /// @brief Returns the address of the start segment field in the BIOS structure.
    uint16_t *getStartSegmentAddr() const
    {
        return (uint16_t *)(static_cast<uint8_t *>(m_pStart) + 6);
    }

    /// @brief Returns the address of the release date string field in the BIOS structure.
    uint8_t *getReleaseDateStringAddr() const
    {
        return (uint8_t *)(static_cast<uint8_t *>(m_pStart) + 8);
    }

    /// @brief Returns the address of the ROM size field in the BIOS structure.
    uint8_t *getRomSizeAddr() const
    {
        return (uint8_t *)(static_cast<uint8_t *>(m_pStart) + 9);
    }

    /// @brief Returns the address of the characteristics field in the BIOS structure.
    uint64_t *getCharacteristicsAddr() const
    {
        return (uint64_t *)(static_cast<uint8_t *>(m_pStart) + 10);
    }

    /// @brief Returns the address of the extension byte field in the BIOS structure.
    uint8_t *getExtensionByteAddr() const
    {
        return (uint8_t *)(static_cast<uint8_t *>(m_pStart) + 14);
    }

    /// @brief Returns the address of the system BIOS major release field in the BIOS structure.
    uint8_t *getSystemBiosMajorReleaseAddr() const
    {
        return (uint8_t *)(static_cast<uint8_t *>(m_pStart) + getLength() - 4);
    }

    /// @brief Returns the address of the system BIOS minor release field in the BIOS structure.
    uint8_t *getSystemBiosMinorReleaseAddr() const
    {
        return getSystemBiosMajorReleaseAddr() + 1;
    }

    /// @brief Returns the address of the embedded controller firmware major release field in the BIOS structure.
    uint8_t *getEmbeddedControllerFirmwareMajorReleaseAddr() const
    {
        return getSystemBiosMinorReleaseAddr() + 1;
    }

    /// @brief Returns the address of the embedded controller firmware minor release field in the BIOS structure.
    uint8_t *getEmbeddedControllerFirmwareMinorReleaseAddr() const
    {
        return getEmbeddedControllerFirmwareMajorReleaseAddr() + 1;
    }
};

#endif /* AB42FE93_A33B_4AA5_9CF2_CBC263FCA5AE */
