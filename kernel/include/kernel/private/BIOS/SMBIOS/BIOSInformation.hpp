#ifndef AB42FE93_A33B_4AA5_9CF2_CBC263FCA5AE
#define AB42FE93_A33B_4AA5_9CF2_CBC263FCA5AE

#include <stdint.h>
#include <stdio.h>
#include "kernel/private/BIOS/BIOSStructureCommon.h"

class BIOSInformation
{
public:
    BIOSInformation() = default;
    BIOSInformation(void *p_start)
    {
        ScanBIOSInformation(p_start);
    };

    void* ScanBIOSInformation(void *p_start)
    {
        m_pStart = p_start;

        int end = 1;
        uint8_t *ptr = getEmbeddedControllerFirmwareMinorReleaseAddr()+1;

        for (; ptr[end - 1] != '\0' || ptr[end] != '\0'; end++)
            ;

        m_pEnd = ptr + end + 1;

        extractStringAddress((const char *)ptr, (const char *)ptr + end, m_strings, StringCount);

        return m_pEnd;
    }

    void *getStartAddr()
    {
        return m_pStart;
    }

    void *getEndAddr()
    {
        return m_pEnd;
    }

    uint8_t getType()
    {
        return *getTypeAddr();
    }

    uint8_t getLength()
    {
        return *getLengthAddr();
    }

    uint16_t getHandle()
    {
        return *getHandleAddr();
    }

    uint8_t getVendorString()
    {
        return *getVendorStringAddr();
    }

    uint8_t getVersionString()
    {
        return *getVersionStringAddr();
    }

    uint16_t getStartSegment()
    {
        return *getStartSegmentAddr();
    }

    uint8_t getReleaseDateString()
    {
        return *getReleaseDateStringAddr();
    }

    uint8_t getRomSize()
    {
        return *getRomSizeAddr();
    }

    uint64_t getCharacteristics()
    {
        return *getCharacteristicsAddr();
    }

    uint8_t getSystemBiosMajorRelease()
    {
        return *getsystemBiosMajorReleaseAddr();
    }

    uint8_t getSystemBiosMinorRelease()
    {
        return *getsystemBiosMinorReleaseAddr();
    }

    uint8_t getEmbeddedControllerFirmwareMajorRelease()
    {
        return *getEmbeddedControllerFirmwareMajorReleaseAddr();
    }

    uint8_t getEmbeddedControllerFirmwareMinorRelease()
    {
        return *getEmbeddedControllerFirmwareMinorReleaseAddr();
    }

    void print()
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
            printf("%s ", m_strings[i]);
        }
        printf("\n");
    }

private:
    void *m_pStart{};
    void *m_pEnd{};

     /*
        vendorString;
        versionString;
        releaseDateString;
    */
    static constexpr uint8_t StringCount = 3;
    const char *m_strings[StringCount]{};

    uint8_t* getTypeAddr()
    {
        return (uint8_t *)(static_cast<uint8_t *>(m_pStart) + 0);
    }

    uint8_t* getLengthAddr()
    {
        return (uint8_t *)(static_cast<uint8_t *>(m_pStart) + 1);
    }

    uint16_t* getHandleAddr()
    {
        return (uint16_t *)(static_cast<uint8_t *>(m_pStart) + 2);
    }

    uint8_t* getVendorStringAddr()
    {
        return (uint8_t *)(static_cast<uint8_t *>(m_pStart) + 4);
    }

    uint8_t* getVersionStringAddr()
    {
        return (uint8_t *)(static_cast<uint8_t *>(m_pStart) + 5);
    }

    uint16_t* getStartSegmentAddr()
    {
        return (uint16_t *)(static_cast<uint8_t *>(m_pStart) + 6);
    }

    uint8_t* getReleaseDateStringAddr()
    {
        return (uint8_t *)(static_cast<uint8_t *>(m_pStart) + 8);
    }

    uint8_t* getRomSizeAddr()
    {
        return (uint8_t *)(static_cast<uint8_t *>(m_pStart) + 9);
    }

    uint64_t* getCharacteristicsAddr()
    {
        return (uint64_t *)(static_cast<uint8_t *>(m_pStart) + 10);
    }

    uint8_t* getExtensionByteAddr()
    {
        return (uint8_t *)(static_cast<uint8_t *>(m_pStart) + 14);
    }

    uint8_t* getsystemBiosMajorReleaseAddr()
    {
        return (uint8_t *)(static_cast<uint8_t *>(m_pStart) + getLength() - 4);
    }

    uint8_t* getsystemBiosMinorReleaseAddr()
    {
        return getsystemBiosMajorReleaseAddr() + 1;
    }

    uint8_t* getEmbeddedControllerFirmwareMajorReleaseAddr()
    {
        return getsystemBiosMinorReleaseAddr() + 1;
    }

    uint8_t* getEmbeddedControllerFirmwareMinorReleaseAddr()
    {
        return getEmbeddedControllerFirmwareMajorReleaseAddr() + 1;
    }
};

#endif /* AB42FE93_A33B_4AA5_9CF2_CBC263FCA5AE */
