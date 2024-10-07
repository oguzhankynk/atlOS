/**
 * @file FAT16.hpp
 * @author Oguzhan KAYNAK
 * @brief FAT16 File System Organization
 *
 * Copyright © 2024 Oguzhan KAYNAK
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the “Software”), to deal in the Software
 * without  restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit
 * persons to whom the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
 * PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#ifndef A9056C1A_5F96_452E_A08C_D83E9CDC8727
#define A9056C1A_5F96_452E_A08C_D83E9CDC8727

#include <stdint.h>
#include <FileSystem/FileSystem.hpp>

 /**
  * @brief Disk organization of the FAT16 file system
  *
  *  +-------------+-------------------+-------------------+--------------------------+
  *  | Boot Sector |   FAT tables      |  Root Directory   |        Data Area         |
  *  |  (Sector 0) | FAT1    | FAT2    |                   |                          |
  *  |  + Reserved |         |         |                   |      File Clusters       |
  *  |    Sectors  |         |         |                   |                          |
  *  +-------------+---------+---------+-------------------+--------------------------+
  *
  *  + The Boot Sector is the first sector within the reserved region
  *  and contains essential information about the file system. Look: @struct BOOTSector
  *
  *  + The FAT structure uses linked lists to manage files.
  *  Each file or directory entry in a (sub)directory contains a cluster number for the first chunk.
  *  This cluster number corresponds to an entry in the FAT, where a word value either points to
  *  the next cluster or marks the end of the file. Valid values are:
  *  0x0000: Free cluster
  *  0x0001 - 0x0002: Not allowed
  *  0x0003 - 0xFFEF: Next cluster number
  *  0xFFF7: Cluster contains bad sectors
  *  0xFFF8 - 0xFFFF: End-of-file
  *
  *
  *  + Each entry in a directory list is 32 bytes long.
  *  The root directory is the only directory located at a fixed position and
  *  the only one that can contain an entry with the Volume Label attribute.
  *  Its size is specified in the @struct BPB. Look: @struct Directory
  *
  *  The Data Region is divided into clusters, with each cluster having a corresponding
  *  entry in the FAT. This entry either points to the next cluster with file data or
  *  contains an End-of-file value, indicating no further clusters.
  *
  *  For more details : https://wiki.osdev.org/FAT
  */

class Disk;
struct Handle;
struct FILE;

namespace FileSystem
{
	namespace FAT16
	{

		/**
		 * @brief BPB (BIOS Parameter Block)
		 *
		 */
		struct BPB
		{
			uint8_t 		JMPInstruction[3]{};	/// JMP SHORT 3C NOP
			uint8_t			OEMIdentifier[8]{};		/// OEM identifier
			uint16_t		bytesPerSector{};		/// The number of Bytes per sector
			uint8_t			sectorsPerCluster{};	/// Number of sectors per cluster.
			uint16_t		reservedSectors{};		/// Number of reserved sectors. The boot record sectors are included in this value.
			uint8_t			numberOfFats{};			/// Number of File Allocation Tables (FAT's) on the storage media.
			uint16_t		numDirEntries{};		/// Number of root directory entries (must be set so that the root directory occupies entire sectors).
			uint16_t		totalSectors{};			/// The total sectors in the logical volume. If this value is 0, 
			/// it means there are more than 65535 sectors in the volume, 
			/// and the actual count is stored in the Large Sector Count entry at 0x20.
			uint8_t			mediaDescriptorType{}; 	/// Media Descriptor Type
			uint16_t		sectorsPerFat{};		/// Number of sectors per FAT.
			uint16_t		sectorsPerTrack{};		/// Number of sectors per track.
			uint16_t		numberOfHeads{};		/// Number of heads or sides on the storage media.
			uint32_t		hiddenSectors{};		/// Number of hidden sectors. 
			uint32_t		longSectors{};			/// Large sector count. This field is set if there are more than 65535 sectors in the volume,
			/// resulting in a value which does not fit in the Number of Sectors entry at 0x13.

			void print() const noexcept;

		} __attribute__((packed));



		/**
		 * @brief The Boot Sector is the first sector of a storage device
		 * that contains filesystem metadata
		 *
		 */
		struct BOOTSector
		{
			BPB	Bpb{};					/// BPB structure
			uint8_t driveNumber{}; 		/// Drive number. The value here should be identical to the value returned by BIOS interrupt 0x13, or passed in the DL register;
			uint8_t reserved{};			/// Reserved .
			uint8_t bootSignature{}; 		/// Signature (must be 0x28 or 0x29).
			uint32_t volumeID{};			/// Used for tracking volumes
			uint8_t volumeLabel[11]{};	/// Volume label string.
			uint8_t fileSystemType[8]{};	/// System identifier string. This field is a string representation of the FAT file system type.  62
			uint8_t bootCode[448]{};		/// Boot code.
			uint16_t signature{};			/// Bootable partition signature 0xAA55.

			void print() const noexcept;

		} __attribute__((packed));


		/**
		 * @brief A Directory is a structure in a filesystem that stores
		 * metadata about files and subdirectories, including their names, sizes,
		 * and locations within the storage medium.
		 *
		 */
		struct Directory
		{
			uint8_t   fileName[8];           /// Filename
			uint8_t   extension[3];          /// Extension
			uint8_t   attributes;            /// File attributes
			uint16_t  reserved;				 /// Reserved 
			uint16_t  creationTime;          /// Creation Time
			uint16_t  creationDate;			 /// creation Date
			uint16_t  lastAccessDate;        /// Last Access Date 
			uint16_t  reserved2; 	         /// Ignore in FAT16
			uint16_t  lastWriteime;          /// Last Write Time
			uint16_t  lastWriteDate; 		 /// Last Write Date
			uint16_t  firstCluster;          /// First Logical Cluster 
			uint32_t  fileSize;              /// File size in bytes

			void print() const noexcept;
		};

		enum class Attributes : uint8_t
		{
			READ_ONLY = (1 << 0), /// Prevents programs from overwriting or deleting the entry.
			HIDDEN = (1 << 1), ///  Hides the file in normal directory listings, though it can be overridden by users.
			SYSTEM = (1 << 2), /// Marks files/directories as critical, discouraging modification.
			VOLUME = (1 << 3), /// Indicates a directory entry for a volume label, not a file, pointing to cluster 0
			DIRECTORY = (1 << 4), /// Points to a sub-directory rather than a file, with the format identical to the root directory.
			ARCHIVE = (1 << 5) /// Used by backup utilities to track file changes (created, renamed, or modified).
		};

		static constexpr int O_RDONLY = 0x00;
		static constexpr int O_WRONLY = 0x01;
		static constexpr int O_RDWR = 0x02;
		static constexpr int O_CREAT = 0x04;

		struct FAT16File : public File
		{
			Directory* dirEntry{};		/// Pointer to the directory entry
			uint32_t startCluster{};    /// The cluster where the file starts.
			uint32_t startSector{};     /// The sector where the file starts.
			uint32_t endCluster{};		///	The cluster where the file end.
			uint32_t endSector{};		/// The sector where the file ends.
			uint32_t currentCluster{};	/// Current cluster for reading
			uint32_t fileIndex{};  		/// Root directory index of file
		};

		class FAT16FileSystem : public IFileSystem
		{

		private:
			bool m_mounted{};

			uint32_t m_StartingSector{};
			BOOTSector m_sector{};

			uint32_t m_totalSectors{};         /// The total number of sectors
			uint16_t m_fatSize{};              /// Number of sectors per FAT.
			uint32_t m_rootDirSectors{};      /// The number of sectors allocated for the root directory
			uint32_t m_firstDataSector{};     /// The starting sector of the data area
			uint32_t m_firstFatSector{};      /// The starting sector of the FAT
			uint32_t m_dataSectors{};          /// The total number of sectors available for data    
			uint32_t m_totalClusters{};        /// The total number of clusters available for file allocation        
			uint32_t m_firstRootDirSector{}; /// The starting sector of the root directory            
			uint32_t m_rootDirectorySize{};   /// The size of the root directory in bytes

			uint32_t m_bytesPerSector{};
			uint32_t m_numDirEntries{};
			uint32_t m_clusterSize{};

			uint8_t* m_rootDirectoryBuffer{}; ///   
			Directory* m_dirEntry{};

			void formatFilename(const char* filename, char* formattedName) const;
			uint32_t findNextCluster(uint32_t cluster) const;
			uint32_t clusterToSector(uint32_t cluster) const;
			uint16_t findFreeCluster() const;
			void updateFAT(uint16_t cluster, uint16_t nextCluster);
			void updateDirectoryEntry(size_t entryIndex);

		public:
			FAT16FileSystem(const Disk& device, uint32_t lbaFirsSector);
			~FAT16FileSystem();

			bool mount() override;
			File* open(const char* filename) override;
			ssize_t read(File* file, void* buffer, size_t size) override;
			ssize_t write(File* file, const void* buffer, size_t size) override;
			int close(File* file) override;
			int remove(const char* filename) override;
			void printFilelist() override;
		};

	}//namespace FAT16
} // namespace FileSystem
#endif /* A9056C1A_5F96_452E_A08C_D83E9CDC8727 */
