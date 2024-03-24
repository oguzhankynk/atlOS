#include <kernel/BIOS.h>

#include <stdio.h>
#include <string.h>

static void printSMBIOSEntryPoint(const SMBIOSEntryPoint *handle) ;
static void printBIOSInformation(const BIOSInfo* info);


static void BIOSInformation(const void* startAddr);

void SMBIOSInit(SMBIOSEntryPoint* handle)
{
    volatile const char* ptr = (volatile void*)0x000F0000;
    int length = 0;

	for (; ptr < 0x000FFFFF; ptr+=16)
	{
		if (memcmp(ptr, "_SM_", 4)==0)
		{
            length = ptr[5];
            uint8_t checksum = 0;

            for (int i = 0; i < length; i++)
                checksum += ptr[i];
 
            if (checksum == 0)
            {
                //printf("Found at %p \n", ptr);
                break;
            }
		}
	}

    if ((unsigned int) ptr == 0x00100000)
    {
        /* Error, SMBIOS could not be located */
        return -1;
    }

	memcpy(handle->signature, ptr, 4);
    handle->checksum = *(ptr + 4);
    handle->length = *(ptr + 5);
    handle->majorVersion = *(ptr + 6);
    handle->minorVersion = *(ptr + 7);
    handle->maximumStructureSize = *(uint16_t*)(ptr + 8);
    handle->entryPointRevision = *(ptr + 10);
    memcpy(handle->formattedArea, ptr + 11, 5);
    memcpy(handle->intermediateAnchor, ptr + 16, 5);
    handle->intermediateChecksum = *(ptr + 21);
    handle->structureTableLength = *(uint16_t*)(ptr + 22);
    handle->structureTableAddress = *(uint32_t*)(ptr + 24);
    handle->numberOfStructures = *(uint16_t*)(ptr + 28);
    handle->bcdRevision = *(ptr + 30);

    //printSMBIOSEntryPoint(handle);

    
    BIOSInformation(handle->structureTableAddress);

    
}

static void printSMBIOSEntryPoint(const SMBIOSEntryPoint *handle) 
{
    printf("Signature: %c%c%c%c\n", handle->signature[0], handle->signature[1], handle->signature[2], handle->signature[3]);
    printf("Checksum: %d\n", handle->checksum);
    printf("Length: %u\n", handle->length);
    printf("Major Version: %u\n", handle->majorVersion);
    printf("Minor Version: %u\n", handle->minorVersion);
    printf("Maximum Structure Size: %u\n", handle->maximumStructureSize);
    printf("Entry Point Revision: %d\n", handle->entryPointRevision);
    printf("Formatted Area: %c%c%c%c%c\n", handle->formattedArea[0], handle->formattedArea[1], handle->formattedArea[2], handle->formattedArea[3], handle->formattedArea[4]);
    printf("Intermediate Anchor: %c%c%c%c%c\n", handle->intermediateAnchor[0], handle->intermediateAnchor[1], handle->intermediateAnchor[2], handle->intermediateAnchor[3], handle->intermediateAnchor[4]);
    printf("Intermediate Checksum: %d\n", handle->intermediateChecksum);
    printf("Structure Table Length: %u\n", handle->structureTableLength);
    printf("Structure Table Address: %p\n", handle->structureTableAddress);
    printf("Number of Structures: %d\n", handle->numberOfStructures);
    printf("BCD Revision: %d\n", handle->bcdRevision);
}

static void BIOSInformation(const void* startAddr)
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

    ptr = startAddr + info.length -4;

    info.systemBiosMajorRelease = *ptr++;

    info.systemBiosMinorRelease = *ptr++;

    info.embeddedControllerFirmwareMajorRelease = *ptr++;

    info.embeddedControllerFirmwareMinorRelease = *ptr++;

    printBIOSInformation(&info);

    printMemory(ptr, 32);
    int strLen =1; 
    while(ptr[strLen-1]!='\0' || ptr[strLen]!='\0')
    {
        strLen++;
    }

    printf("strlen = :%u \n", strLen +1);

    const char* vendorString = ptr;
    ptr+= strlen(ptr) + 1;

    const char* versionString = ptr;
    ptr+= strlen(ptr) + 1;

    const char* releaseDateString = ptr;
    ptr+= strlen(ptr) + 1;

    printf("%s %s %s", vendorString, versionString, releaseDateString);


}

static void printBIOSInformation(const BIOSInfo* info) 
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