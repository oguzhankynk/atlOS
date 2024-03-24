#ifndef EB497FF7_83CA_4F01_A507_7433482E25D2
#define EB497FF7_83CA_4F01_A507_7433482E25D2

#include <stdint.h>

typedef struct {
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
} SMBIOSEntryPoint;

typedef struct 
    {
        uint8_t type;
        uint8_t length;
        uint16_t handle;
        uint8_t vendorString;  
        uint8_t versionString;  
        uint16_t startSegment;
        uint8_t releaseDateString;    
        uint8_t romSize;
        uint64_t characteristics;
        uint16_t extensionByte1;
        uint8_t systemBiosMajorRelease;
        uint8_t systemBiosMinorRelease;
        uint8_t embeddedControllerFirmwareMajorRelease;
        uint8_t embeddedControllerFirmwareMinorRelease;
    }BIOSInfo;

void SMBIOSInit(SMBIOSEntryPoint* handle);

#endif /* EB497FF7_83CA_4F01_A507_7433482E25D2 */
