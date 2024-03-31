#ifndef B753BF1E_92A3_45A6_ABED_F40B5BE6B059
#define B753BF1E_92A3_45A6_ABED_F40B5BE6B059

#include <stdint.h>
#include <stdio.h>
#include "kernel/private/BIOS/BIOSStructureCommon.h"

class SystemInformation
{
public:
    SystemInformation() = default;
    SystemInformation(void *p_start)
    {
        ScanSystemInformation(p_start);
    };

    void *ScanSystemInformation(void *p_start)
    {
        m_pStart = p_start;

        int end = 1;
        uint8_t *ptr = getFamilyAddr() + 1;

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

    uint8_t getManufacturer()
    {
        return *getManufacturerAddr();
    }

    uint8_t getProductName()
    {
        return *getProductNameAddr();
    }

    uint8_t getVersion()
    {
        return *getVersionAddr();
    }

    uint8_t getSerialNumber()
    {
        return *getSerialNumberAddr();
    }

    const uint8_t *getUuid()
    {
        return getUuidAddr();
    }

    uint8_t getWakeUpType()
    {
        return *getWakeUpTypeAddr();
    }

    uint8_t getSkuNumber()
    {
        return *getSkuNumberAddr();
    }

    uint8_t getFamily()
    {
        return *getFamilyAddr();
    }

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
            printf("%s ", m_strings[i]);
        }
        printf("\n");
    }

private:
    void *m_pStart{};
    void *m_pEnd{};

    /*
        ManufacturerString;
        ProductNameString;
        VersionString;
        SerialNumberString;
        SKUNumberString;
        FamilyString;
    */
    static constexpr uint8_t StringCount = 6;
    const char *m_strings[StringCount]{};

    uint8_t *getTypeAddr()
    {
        return (uint8_t *)(static_cast<uint8_t *>(m_pStart) + 0);
    }

    uint8_t *getLengthAddr()
    {
        return (uint8_t *)(static_cast<uint8_t *>(m_pStart) + 1);
    }

    uint16_t *getHandleAddr()
    {
        return (uint16_t *)(static_cast<uint8_t *>(m_pStart) + 2);
    }

    uint8_t *getManufacturerAddr()
    {
        return (uint8_t *)(static_cast<uint8_t *>(m_pStart) + 4);
    }

    uint8_t *getProductNameAddr()
    {
        return (uint8_t *)(static_cast<uint8_t *>(m_pStart) + 5);
    }

    uint8_t *getVersionAddr()
    {
        return (uint8_t *)(static_cast<uint8_t *>(m_pStart) + 6);
    }

    uint8_t *getSerialNumberAddr()
    {
        return (uint8_t *)(static_cast<uint8_t *>(m_pStart) + 7);
    }

    uint8_t *getUuidAddr()
    {
        return (uint8_t *)(static_cast<uint8_t *>(m_pStart) + 8);
    }

    uint8_t *getWakeUpTypeAddr()
    {
        return (uint8_t *)(static_cast<uint8_t *>(m_pStart) + 24);
    }

    uint8_t *getSkuNumberAddr()
    {
        return (uint8_t *)(static_cast<uint8_t *>(m_pStart) + 25);
    }

    uint8_t *getFamilyAddr()
    {
        return (uint8_t *)(static_cast<uint8_t *>(m_pStart) + 26);
    }
};
#endif /* B753BF1E_92A3_45A6_ABED_F40B5BE6B059 */
