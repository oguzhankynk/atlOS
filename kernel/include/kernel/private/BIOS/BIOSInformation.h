#ifndef AB42FE93_A33B_4AA5_9CF2_CBC263FCA5AE
#define AB42FE93_A33B_4AA5_9CF2_CBC263FCA5AE

#include <stdint.h>

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


void* BIOSInformation(const void* startAddr);
void printBIOSInformation(const BIOSInfo* info);


#endif /* AB42FE93_A33B_4AA5_9CF2_CBC263FCA5AE */
