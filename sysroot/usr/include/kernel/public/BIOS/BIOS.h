#ifndef EB497FF7_83CA_4F01_A507_7433482E25D2
#define EB497FF7_83CA_4F01_A507_7433482E25D2

#include <stdint.h>
#include <kernel/private/BIOS/BIOSInformation.h>
#include <kernel/private/BIOS/SystemInformation.h>

#if defined(__cplusplus)
extern "C" {
#endif

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


void SMBIOSInit(SMBIOSEntryPoint* handle);

#if defined(__cplusplus)
} /* extern "C" */
#endif

#endif /* EB497FF7_83CA_4F01_A507_7433482E25D2 */
