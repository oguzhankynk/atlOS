#include <kernel/public/BIOS/BIOS.h>

#include <stdio.h>
#include <string.h>


static void printSMBIOSEntryPoint(const SMBIOSEntryPoint *handle) ;


void SMBIOSInit(SMBIOSEntryPoint* handle)
{
    const char* ptr = (char*)0x000F0000;
    int length = 0;

	for (; ptr < (const char*)0x000FFFFF; ptr+=16)
	{
		if (memcmp((const void*)ptr, "_SM_", 4)==0)
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
        return ;
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

    
    void* second = BIOSInformation((void*)handle->structureTableAddress);

    void *third = SystemInformation((void*)second);


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

