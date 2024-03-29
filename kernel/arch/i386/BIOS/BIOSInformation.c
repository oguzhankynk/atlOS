#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <kernel/private/BIOS/BIOSInformation.h>

void* BIOSInformation(const void* startAddr)
{
    BIOSInfo info;
    
    uint8_t* ptr = (uint8_t*)startAddr;

    info.type = *ptr++;

    info.length = *ptr++;

    info.handle = *(uint16_t*)ptr;
    ptr += 2;

    info.vendorString = *ptr++;

    info.versionString = *ptr++;

    info.startSegment = *(uint16_t*)ptr;
    ptr += 2;

    info.releaseDateString = *ptr++;

    info.romSize = *ptr++;
    
    info.characteristics = *(uint64_t*)ptr;
    ptr += 8;

    //numberofExtensionByte
    //TODO 

    ptr = (uint8_t *)startAddr + info.length -4;

    info.systemBiosMajorRelease = *ptr++;

    info.systemBiosMinorRelease = *ptr++;

    info.embeddedControllerFirmwareMajorRelease = *ptr++;

    info.embeddedControllerFirmwareMinorRelease = *ptr++;

    //printBIOSInformation(&info);

    int end =1;
    for (; ptr[end - 1] != '\0' || ptr[end] != '\0'; end++);

    void* ret = ptr + end + 1;

    const uint8_t * vendorString = ptr;
    ptr+= strlen((const char*)ptr) + 1;

    const uint8_t * versionString = ptr;
    ptr+= strlen((const char*)ptr) + 1;

    const uint8_t * releaseDateString = ptr;
    ptr+= strlen((const char*)ptr) + 1;

    printf("%s %s %s \n", vendorString, versionString, releaseDateString);

    return ret;
}

void printBIOSInformation(const BIOSInfo* info) 
{
    printf("Type: %u\n", info->type);
    printf("Length: %u\n", info->length);
    printf("Handle: %u\n", info->handle);
    printf("Vendor String: %u\n", info->vendorString);
    printf("Version String: %u\n", info->versionString);
    printf("Start Segment: %u\n", info->startSegment);
    printf("Release Date: %u\n", info->releaseDateString);
    printf("ROM Size: %u\n", info->romSize);
    printf("Characteristics: %llu\n", (unsigned long long)info->characteristics);
    printf("Extension Byte 1: %u\n", info->extensionByte1);
    printf("System BIOS Major Release: %u\n", info->systemBiosMajorRelease);
    printf("System BIOS Minor Release: %u\n", info->systemBiosMinorRelease);
    printf("Embedded Controller Firmware Major Release: %u\n", info->embeddedControllerFirmwareMajorRelease);
    printf("Embedded Controller Firmware Minor Release: %u\n", info->embeddedControllerFirmwareMinorRelease);
}