/**
 * @file SMBIOS.c
 * @author okaynak ()
 * @brief Functions for SMBIOS initialization and management.
 * @date 2024-03-31
 *
 * @copyright Copyright (c) 2024
 *
 */

#include <stdio.h>
#include <string.h>

#include <kernel/private/BIOS/SMBIOS/SMBIOS.hpp>

SMBIOS::SMBIOS()
{
   const char *ptr = (char *)0x000F0000;
   int length = 0;

   for (; ptr < (const char *)0x000FFFFF; ptr += 16)
   {
      if (memcmp((const void *)ptr, "_SM_", 4) == 0)
      {
         length = ptr[5];
         uint8_t checksum = 0;

         for (int i = 0; i < length; i++)
            checksum += ptr[i];

         if (checksum == 0)
         {
            // printf("Found at %p \n", ptr);
            break;
         }
      }
   }

   if (ptr == (char *)0x00100000)
   {
      /* Error, SMBIOS could not be located */
      return;
   }

   memcpy(m_SMBIOSEntryPoint.signature, ptr, 4);
   m_SMBIOSEntryPoint.checksum = *(ptr + 4);
   m_SMBIOSEntryPoint.length = *(ptr + 5);
   m_SMBIOSEntryPoint.majorVersion = *(ptr + 6);
   m_SMBIOSEntryPoint.minorVersion = *(ptr + 7);
   m_SMBIOSEntryPoint.maximumStructureSize = *(uint16_t *)(ptr + 8);
   m_SMBIOSEntryPoint.entryPointRevision = *(ptr + 10);
   memcpy(m_SMBIOSEntryPoint.formattedArea, ptr + 11, 5);
   memcpy(m_SMBIOSEntryPoint.intermediateAnchor, ptr + 16, 5);
   m_SMBIOSEntryPoint.intermediateChecksum = *(ptr + 21);
   m_SMBIOSEntryPoint.structureTableLength = *(uint16_t *)(ptr + 22);
   m_SMBIOSEntryPoint.structureTableAddress = *(uint32_t *)(ptr + 24);
   m_SMBIOSEntryPoint.numberOfStructures = *(uint16_t *)(ptr + 28);
   m_SMBIOSEntryPoint.bcdRevision = *(ptr + 30);

   // printEntryPoint();

   void *pBIOSInformationEnd = m_BIOSInformation.Scan((void *)m_SMBIOSEntryPoint.structureTableAddress);

   void *pSystemInformationEnd = m_SystemInformation.Scan(pBIOSInformationEnd);

   void *pSystemEnclosureChassisEnd = m_SystemEnclosureChassis.Scan(pSystemInformationEnd);

   void *pProcessorInformationEnd = m_ProcessorInformation.Scan(pSystemEnclosureChassisEnd);

   void *pPhysicalMemoryArrayEnd = m_PhysicalMemoryArray.Scan(pProcessorInformationEnd);

   void *pMemoryDeviceEnd = m_MemoryDevice.Scan(pPhysicalMemoryArrayEnd);

   void *pMemoryArrayMappedAddressEnd = m_MemoryArrayMappedAddress.Scan(pMemoryDeviceEnd);

   void *pSystemBootInformationEnd = m_SystemBootInformation.Scan(pMemoryArrayMappedAddressEnd);

   void *pEndOfTableEnd = m_EndOfTable.Scan(pSystemBootInformationEnd);

}

void SMBIOS::printEntryPoint() const
{
   printf("Signature: %c%c%c%c\n", m_SMBIOSEntryPoint.signature[0], m_SMBIOSEntryPoint.signature[1], m_SMBIOSEntryPoint.signature[2], m_SMBIOSEntryPoint.signature[3]);
   printf("Checksum: %d\n", m_SMBIOSEntryPoint.checksum);
   printf("Length: %u\n", m_SMBIOSEntryPoint.length);
   printf("Major Version: %u\n", m_SMBIOSEntryPoint.majorVersion);
   printf("Minor Version: %u\n", m_SMBIOSEntryPoint.minorVersion);
   printf("Maximum Structure Size: %u\n", m_SMBIOSEntryPoint.maximumStructureSize);
   printf("Entry Point Revision: %d\n", m_SMBIOSEntryPoint.entryPointRevision);
   printf("Formatted Area: %c%c%c%c%c\n", m_SMBIOSEntryPoint.formattedArea[0], m_SMBIOSEntryPoint.formattedArea[1], m_SMBIOSEntryPoint.formattedArea[2], m_SMBIOSEntryPoint.formattedArea[3], m_SMBIOSEntryPoint.formattedArea[4]);
   printf("Intermediate Anchor: %c%c%c%c%c\n", m_SMBIOSEntryPoint.intermediateAnchor[0], m_SMBIOSEntryPoint.intermediateAnchor[1], m_SMBIOSEntryPoint.intermediateAnchor[2], m_SMBIOSEntryPoint.intermediateAnchor[3], m_SMBIOSEntryPoint.intermediateAnchor[4]);
   printf("Intermediate Checksum: %d\n", m_SMBIOSEntryPoint.intermediateChecksum);
   printf("Structure Table Length: %u\n", m_SMBIOSEntryPoint.structureTableLength);
   printf("Structure Table Address: %p\n", m_SMBIOSEntryPoint.structureTableAddress);
   printf("Number of Structures: %d\n", m_SMBIOSEntryPoint.numberOfStructures);
   printf("BCD Revision: %d\n", m_SMBIOSEntryPoint.bcdRevision);
}

void SMBIOS::printBIOSInformation() const
{
   m_BIOSInformation.print();
}

void SMBIOS::printSystemInformation() const
{
   m_SystemInformation.print();
}

void SMBIOS::printBaseBoardInformation() const
{
   m_BaseBoardInformation.print();
}

void SMBIOS::printSystemEnclosureChassis() const
{
   m_SystemEnclosureChassis.print();
}

void SMBIOS::printProcessorInformation() const
{
   m_ProcessorInformation.print();
}

void SMBIOS::printPhysicalMemoryArray() const
{
   m_PhysicalMemoryArray.print();
}

void SMBIOS::printMemoryDevice() const
{
   m_MemoryDevice.print();
}

void SMBIOS::printMemoryArrayMappedAddress() const
{
   m_MemoryArrayMappedAddress.print();
}

void SMBIOS::printSystemBootInformation() const
{
   m_SystemBootInformation.print();
}
