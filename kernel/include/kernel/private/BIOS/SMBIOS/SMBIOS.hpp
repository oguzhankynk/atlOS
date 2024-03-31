#ifndef CC04DEBE_B0F3_4321_AA29_F8861E1DEABD
#define CC04DEBE_B0F3_4321_AA29_F8861E1DEABD

#include <kernel/private/BIOS/SMBIOS/BIOSInformation.hpp>
#include <kernel/private/BIOS/SMBIOS/SystemInformation.hpp>

struct SMBIOSEntryPoint
{
    char signature[4];
    uint8_t checksum;
    uint8_t length;
    uint8_t majorVersion;
    uint8_t minorVersion;
    uint16_t maximumStructureSize;
    uint8_t entryPointRevision;
    char formattedArea[5];
    char intermediateAnchor[5];
    uint8_t intermediateChecksum;
    uint16_t structureTableLength;
    uint32_t structureTableAddress;
    uint16_t numberOfStructures;
    uint8_t bcdRevision;
} ;

class SMBIOS
{
    public:
        SMBIOS();
        void printEntryPoint();
        void printBIOSInformation();
        void printSystemInformation();
    private:

        SMBIOSEntryPoint m_SMBIOSEntryPoint;
        BIOSInformation m_BIOSInformation;
        SystemInformation m_SystemInformation;
};
#endif /* CC04DEBE_B0F3_4321_AA29_F8861E1DEABD */
