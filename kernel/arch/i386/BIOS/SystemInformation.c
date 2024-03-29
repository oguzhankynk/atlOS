#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <kernel/private/BIOS/SystemInformation.h>

void* SystemInformation(const void* startAddr) {
    SMBIOSSystemInfo Info;

    // startAddr'dan itibaren bilgileri kopyala
    const uint8_t* ptr = (uint8_t*)startAddr;

    // Type ve Length bilgilerini kopyala
    Info.type = *ptr++;
    Info.length = *ptr++;

    // Handle bilgisini kopyala
    Info.handle = *((uint16_t*)ptr);
    ptr += sizeof(uint16_t);

    // Diğer alanları kopyala (1 byte'lık alanlar)
    Info.manufacturer = *ptr++;
    Info.productName = *ptr++;
    Info.version = *ptr++;
    Info.serialNumber = *ptr++;
    // UUID bilgisini kopyala
    for (size_t i = 0; i < 16; ++i) {
        Info.uuid[i] = *ptr++;
    }

    Info.wakeUpType = *ptr++;
    Info.skuNumber = *ptr++;
    Info.family = *ptr++;

    
    printSystemInformation(&Info);

    int end =1;
    for (; ptr[end - 1] != '\0' || ptr[end] != '\0'; end++);

    const uint8_t* ret = ptr + end + 1;

    const uint8_t* ManufacturerString = ptr;
    ptr+= strlen((const char*)ptr) + 1;

    const uint8_t* ProductNameString = ptr;
    ptr+= strlen((const char*)ptr) + 1;

    printMemory(ptr, 32);

    const uint8_t* VersionString = ptr;
    ptr+= strlen((const char*)ptr) + 1;

    
    const uint8_t* SerialNumberString = ptr;
    ptr+= strlen((const char*)ptr) + 1;

    const uint8_t* SKUNumberString = ptr;
    ptr+= strlen((const char*)ptr) + 1;

    const uint8_t* FamilyString = ptr;
    ptr+= strlen((const char*)ptr) + 1;

    printf("%s \n %s \n %s \n %s \n %s \n %s \n ", ManufacturerString, ProductNameString, VersionString,
        SerialNumberString, SKUNumberString, FamilyString);

    return (void *)ret;
}

void printSystemInformation(const SMBIOSSystemInfo* info) {
    printf("Type: %u\n", info->type);
    printf("Length: %u\n", info->length);
    printf("Handle: %u\n", info->handle);
    printf("Manufacturer: %u\n", info->manufacturer);
    printf("Product Name: %u\n", info->productName);
    printf("Version: %u\n", info->version);
    printf("Serial Number: %u\n", info->serialNumber);
    printf("UUID: ");
    for (size_t i = 0; i < 16; ++i) {
        printf("%02X ", info->uuid[i]);
    }
    printf("\n");
    printf("Wake-up Type: %u\n", info->wakeUpType);
    printf("SKU Number: %u\n", info->skuNumber);
    printf("Family: %u\n", info->family);
}
