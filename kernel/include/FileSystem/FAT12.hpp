/**
 * @file FAT12.hpp
 * @author Oguzhan KAYNAK 
 * @brief 
 * 
 * Copyright © 2024 Oguzhan KAYNAK
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the “Software”), to deal in the Software *without  restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit *persons to whom the Software is furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR *PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR *OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * 
 */
#ifndef A9056C1A_5F96_452E_A08C_D83E9CDC8727
#define A9056C1A_5F96_452E_A08C_D83E9CDC8727

#include <stdint.h>

namespace FAT12
{

struct BIOSPARAMATERBLOCK 
{

	uint8_t			OEMName[8];
	uint16_t		BytesPerSector;
	uint8_t			SectorsPerCluster;
	uint16_t		ReservedSectors;
	uint8_t			NumberOfFats;
	uint16_t		NumDirEntries;
	uint16_t		NumSectors;
	uint8_t			Media;
	uint16_t		SectorsPerFat;
	uint16_t		SectorsPerTrack;
	uint16_t		HeadsPerCyl;
	uint32_t		HiddenSectors;
	uint32_t		LongSectors;

} __attribute__((packed));

using PBIOSPARAMATERBLOCK = BIOSPARAMATERBLOCK*;

struct BIOSPARAMATERBLOCKEXT 
{

	uint32_t			SectorsPerFat32;   ///sectors per FAT
	uint16_t			Flags;             ///flags
	uint16_t			Version;           ///version
	uint32_t			RootCluster;       ///starting root directory
	uint16_t			InfoCluster;	   ///
	uint16_t			BackupBoot;        ///location of bootsector copy
	uint16_t			Reserved[6]; 	   ///

} __attribute__((packed));

using PBIOSPARAMATERBLOCKEXT = BIOSPARAMATERBLOCKEXT*;

struct BOOTSECTOR 
{

	uint8_t			Ignore[3];		/// jmp instruction
	BIOSPARAMATERBLOCK	Bpb;		///BPB structure
	BIOSPARAMATERBLOCKEXT	BpbExt;	///extended BPB info
	uint8_t			Filler[448];	///needed to make struct 512 bytes

} __attribute__((packed));

using PBOOTSECTOR = BOOTSECTOR*;

struct MOUNT_INFO 
{

	uint32_t numSectors;
	uint32_t fatOffset;
	uint32_t numRootEntries;
	uint32_t rootOffset;
	uint32_t rootSize;
	uint32_t fatSize;
	uint32_t fatEntrySize;

} ;

using PMOUNT_INFO = MOUNT_INFO*;

struct DIRECTORY 
{
	uint8_t   Filename[8];           //filename
	uint8_t   Ext[3];                //extension (8.3 filename format)
	uint8_t   Attrib;                //file attributes
	uint8_t   Reserved;
	uint8_t   TimeCreatedMs;         //creation time
	uint16_t  TimeCreated;
	uint16_t  DateCreated;           //creation date
	uint16_t  DateLastAccessed;
	uint16_t  FirstClusterHiBytes;
	uint16_t  LastModTime;           //last modification date/time
	uint16_t  LastModDate;
	uint16_t  FirstCluster;          //first cluster of file data
	uint32_t  FileSize;              //size in bytes

} ;

using PDIRECTORY = DIRECTORY*;


}//namespace FAT12
#endif /* A9056C1A_5F96_452E_A08C_D83E9CDC8727 */
