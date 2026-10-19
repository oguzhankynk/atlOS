/**
 * @file fat16.cpp
 * @author Oguzhan KAYNAK
 * @brief
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

#include <fs/fat16.hpp>

#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include <core/logger.hpp>
#include <drivers/disk/disk.hpp>
#include <lib/utils.hpp>
#include <mm/heap.hpp>

namespace FileSystem {
namespace FAT16 {
    /// Enable / Disable logs
    static constexpr LogFlag dbgLog = LogFlag::Log;
    static constexpr LogFlag infLog = LogFlag::Log;
    static constexpr LogFlag errLog = LogFlag::Log;

    void BPB::print() const noexcept
    {
        printf("JMP Instruction: 0x%02X 0x%02X 0x%02X\n",
            JMPInstruction[0], JMPInstruction[1], JMPInstruction[2]);
        printf("OEM Identifier: %.8s\n", OEMIdentifier);
        printf("Bytes Per Sector: %u\n", bytesPerSector);
        printf("Sectors Per Cluster: %u\n", sectorsPerCluster);
        printf("Reserved Sectors: %u\n", reservedSectors);
        printf("Number of FATs: %u\n", numberOfFats);
        printf("Number of Root Directory Entries: %u\n", numDirEntries);
        printf("Total Sectors: %u\n", totalSectors);
        printf("Media Descriptor Type: 0x%02X\n", mediaDescriptorType);
        printf("Sectors Per FAT: %u\n", sectorsPerFat);
        printf("Sectors Per Track: %u\n", sectorsPerTrack);
        printf("Number of Heads: %u\n", numberOfHeads);
        printf("Hidden Sectors: %u\n", hiddenSectors);
        printf("Large Sectors: %u\n", longSectors);
    }

    void BOOTSector::print() const noexcept
    {
        Bpb.print();
        printf("Drive Number: 0x%02X\n", driveNumber);
        printf("Boot Signature: 0x%02X\n", bootSignature);
        printf("Volume ID: 0x%08X\n", volumeID);
        printf("Volume Label: %.11s\n", volumeLabel);
        printf("File System Type: %.8s\n", fileSystemType);
        printf("Signature: 0x%04X\n", signature);
    }

    void Directory::print() const noexcept
    {
        printf("Name: %.8s.%.3s\n", fileName, extension);
        printf("Attributes: 0x%02X Last Access Date: %04u\n", attributes, lastAccessDate);
        printf("Creation Time: %04u Creation Date: %04u\n", creationTime, creationDate);
        printf("Last Write Time: %04u Last Write Date: %04u\n", lastWriteime, lastWriteDate);
        printf("First Cluster: %u File Size: %u bytes\n", firstCluster, fileSize);
    }

    FAT16FileSystem::FAT16FileSystem(const Disk& device, uint32_t lbaFirsSector)
        : IFileSystem(device, lbaFirsSector)
    {
        m_StartingSector = lbaFirsSector;
    }

    FAT16FileSystem::~FAT16FileSystem()
    {
        if (m_mounted) {
            kfree(m_rootDirectoryBuffer);
        }
    }

    bool FAT16FileSystem::mount()
    {
        m_device.readSector(m_StartingSector, (uint8_t*)&m_sector);

        if (m_sector.signature == 0xAA55) {
            logMsg(dbgLog, "Disk signature OK \n");
        } else {
            logMsg(errLog, "Disk signature error \n");
            return false;
        }

        m_bytesPerSector = m_sector.Bpb.bytesPerSector;
        m_clusterSize = m_sector.Bpb.sectorsPerCluster * m_sector.Bpb.bytesPerSector;
        m_numDirEntries = m_sector.Bpb.numDirEntries;
        m_totalSectors = (m_sector.Bpb.totalSectors == 0) ? m_sector.Bpb.longSectors : m_sector.Bpb.totalSectors;

        m_fatSize = m_sector.Bpb.sectorsPerFat;
        m_rootDirSectors = ((m_numDirEntries * 32) + (m_bytesPerSector - 1)) / m_bytesPerSector;
        m_rootDirectorySize = m_rootDirSectors * m_bytesPerSector;

        m_dataSectors = m_totalSectors - (m_sector.Bpb.reservedSectors + (m_sector.Bpb.numberOfFats * m_fatSize) + m_rootDirSectors);
        m_totalClusters = m_dataSectors / m_sector.Bpb.sectorsPerCluster;

        m_firstFatSector = m_StartingSector + m_sector.Bpb.reservedSectors;
        m_firstRootDirSector = m_firstFatSector + (m_sector.Bpb.numberOfFats * m_fatSize);
        m_firstDataSector = m_firstFatSector + (m_sector.Bpb.numberOfFats * m_fatSize) + m_rootDirSectors;

        logMsg(dbgLog, "total_sectors    :%05u  fat_size           :%05u \n", m_totalSectors, m_fatSize);
        logMsg(dbgLog, "numberOfFats     :%05u  clusterSize        :%05u \n", m_sector.Bpb.numberOfFats, m_clusterSize);
        logMsg(dbgLog, "root_dir_sectors :%05u  first_data_sector  :%05u \n", m_rootDirSectors, m_firstDataSector);
        logMsg(dbgLog, "first_fat_sector :%05u  data_sectors       :%05u \n", m_firstFatSector, m_dataSectors);
        logMsg(dbgLog, "total_clusters   :%05u  root_directory_size:%05u \n", m_totalClusters, m_rootDirectorySize);
        logMsg(dbgLog, "numDirEntries    :%05u  bytesPerSector     :%05u \n", m_numDirEntries, m_bytesPerSector);

        m_rootDirectoryBuffer = (uint8_t*)kmalloc(m_rootDirectorySize);

        if (m_rootDirectoryBuffer == nullptr) {
            logMsg(errLog, "NO memory for root directory! \n");
            return false;
        }

        memset(m_rootDirectoryBuffer, 0, m_rootDirectorySize);

        for (int i = 0; i < m_rootDirSectors; ++i) {
            m_device.readSector(m_firstRootDirSector + i, m_rootDirectoryBuffer + (i * m_bytesPerSector));
        }

        m_dirEntry = reinterpret_cast<Directory*>(m_rootDirectoryBuffer);

        m_mounted = true;

        return true;
    }

    void FAT16FileSystem::printFilelist()
    {

        for (size_t i = 0; i < m_numDirEntries; ++i) {
            auto& dirEntry = m_dirEntry[i];

            if (dirEntry.fileName[0] == 0x00) {
                continue;
            }

            if (dirEntry.fileName[0] == 0xE5) {
                continue;
            }

            printf("File: %.8s.%.3s\n", dirEntry.fileName, dirEntry.extension);
        }
    }

    File* FAT16FileSystem::open(const char* filename)
    {

        char formattedName[12]; // 8+1+3

        formatFilename(filename, formattedName);

        FAT16File* fd = (FAT16File*)kmalloc(sizeof(FAT16File));

        if (fd == nullptr) {
            logMsg(errLog, "No memory for open a file\n");
            return nullptr;
        }

        bool newFile = true;

        auto fileIndex = 0;

        for (; fileIndex < m_numDirEntries; ++fileIndex) {

            auto& dirEntry = m_dirEntry[fileIndex];

            if (dirEntry.fileName[0] == 0x00) {
                break;
            }

            if (dirEntry.fileName[0] == 0xE5) {
                break;
            }

            bool searchchFlag = true;

            for (int j = 0; j < 11; j++) {
                if (dirEntry.fileName[j] != formattedName[j]) {
                    searchchFlag = false;
                    break;
                }
            }

            if (searchchFlag) {
                logMsg(dbgLog, "File:%s found. file indx: %u \n", formattedName, fileIndex);

                fd->dirEntry = &dirEntry;
                fd->fileIndex = fileIndex;
                fd->startCluster = dirEntry.firstCluster; // Starting cluster
                fd->startSector = (fd->startCluster - 2) * m_sector.Bpb.sectorsPerCluster + m_firstDataSector;
                fd->currentCluster = fd->startCluster;
                fd->position = 0; // Start at the beginning of the file
                fd->size = dirEntry.fileSize; // Size of the file

                // find end sector

                uint16_t currentCluster = fd->startCluster;
                uint16_t previousCluster = 0;

                while (currentCluster > 0x0002 && currentCluster < 0xFFF8) {
                    previousCluster = currentCluster;
                    currentCluster = findNextCluster(currentCluster);
                }

                uint32_t lastClusterStartSector = (previousCluster - 2) * m_sector.Bpb.sectorsPerCluster + m_firstDataSector;

                uint32_t totalSectors = (fd->size + m_sector.Bpb.bytesPerSector - 1) / m_sector.Bpb.bytesPerSector;

                fd->endCluster = previousCluster;
                fd->endSector = lastClusterStartSector + (totalSectors - 1) % m_sector.Bpb.sectorsPerCluster;

                logMsg(dbgLog, "startCluster:%u startSector:%u \nendCluster:%u endSector:%u size:%u \n",
                    fd->startCluster, fd->startSector, fd->endCluster, fd->endSector, fd->size);

                newFile = false;
            }
        }

        if (newFile == true) {
            auto newCluster = findFreeCluster();

            logMsg(dbgLog, "File  not found. It will be created. newCluster:%u \n", newCluster);

            if (newCluster == 0xFFFF) {
                logMsg(dbgLog, "File:%s No memory for new cluster\n", formattedName);
                kfree(fd);
                return nullptr;
            }

            updateFAT(newCluster, 0xFFFF);

            auto& dirEntry = m_dirEntry[fileIndex];

            memset(&dirEntry, 0, sizeof(Directory));
            memcpy(dirEntry.fileName, formattedName, 11);
            dirEntry.attributes = static_cast<uint8_t>(Attributes::ARCHIVE);
            dirEntry.firstCluster = newCluster;
            dirEntry.fileSize = 0;

            fd->dirEntry = &dirEntry;
            fd->fileIndex = fileIndex;
            fd->startCluster = dirEntry.firstCluster;
            fd->startSector = (fd->startCluster - 2) * m_sector.Bpb.sectorsPerCluster + m_firstDataSector;
            fd->currentCluster = fd->startCluster;
            fd->position = 0; // Start at the beginning of the file
            fd->size = dirEntry.fileSize; // Size of the file
            fd->endCluster = fd->startCluster;
            fd->endSector = fd->startSector;

            updateDirectoryEntry(fileIndex);
        }

        return fd;
    }

    int FAT16FileSystem::close(File* fd)
    {
        if (fd == nullptr) {
            return -1;
        }

        kfree((FAT16File*)fd);

        return 0;
    }

    int FAT16FileSystem::remove(const char* filename)
    {
        char formattedName[12]; // 8+1+3

        formatFilename(filename, formattedName);

        auto fileIndex = 0;

        for (; fileIndex < m_numDirEntries; ++fileIndex) {
            auto& dirEntry = m_dirEntry[fileIndex];

            bool searchchFlag = true;

            for (int j = 0; j < 11; j++) {
                if (dirEntry.fileName[j] != formattedName[j]) {
                    searchchFlag = false;
                    break;
                }
            }

            if (searchchFlag) {
                uint16_t currentCluster = dirEntry.firstCluster;

                while (currentCluster < 0xFFF8) {
                    uint16_t nextCluster = findNextCluster(currentCluster);
                    updateFAT(currentCluster, 0x00);
                    currentCluster = nextCluster;
                }

                memset(&dirEntry, 0, sizeof(dirEntry));
                updateDirectoryEntry(fileIndex);
                return 0;
            }
        }

        return -1; // file could not found
    }

    ssize_t FAT16FileSystem::read(File* file, void* buffer, size_t size)
    {
        if (file == nullptr) {
            return -1;
        }

        FAT16File* fat16File = static_cast<FAT16File*>(file);
        Directory* dirEntry = fat16File->dirEntry;

        if (fat16File->position >= dirEntry->fileSize) {
            return 0; // End of file reached
        }

        // Adjust count if it exceeds remaining file size
        size_t bytesToRead = size;
        if (fat16File->position + bytesToRead > dirEntry->fileSize) {
            bytesToRead = dirEntry->fileSize - fat16File->position;
        }

        auto clusterOffset = fat16File->position % (m_bytesPerSector * m_sector.Bpb.sectorsPerCluster);

        uint32_t sectorIndexInCluster = clusterOffset / m_bytesPerSector;

        uint32_t sectorOffset = clusterOffset % m_bytesPerSector;

        size_t bytesRead = 0;

        logMsg(dbgLog, "ReadFile: count:%u position:%u f_size: %u bytes_to_read: %u cluster:%u sectorIndex:%u sectorOffset:%u \n",
            size, fat16File->position, dirEntry->fileSize, bytesToRead, fat16File->currentCluster, sectorIndexInCluster, sectorOffset);

        while (bytesRead < bytesToRead) {
            uint32_t lba = clusterToSector(fat16File->currentCluster) + sectorIndexInCluster;

            // Read the sector containing file data into the buffer

            uint8_t readBuffer[512];

            m_device.readSector(lba, readBuffer);

            auto readSize = 512 - sectorOffset;
            readSize = Utils::min((size_t)readSize, bytesToRead - bytesRead);

            memcpy((char*)buffer + bytesRead, readBuffer + sectorOffset, readSize);

            // Increment read_bytes and offset accordingly
            bytesRead += readSize;

            // Check if we need to read from the next cluster
            if (bytesRead >= bytesToRead) {
                break;
            }

            sectorOffset = 0;

            // If we reached the end of the cluster, move to the next cluster
            sectorIndexInCluster++;
            if (sectorIndexInCluster >= m_sector.Bpb.sectorsPerCluster) {
                fat16File->currentCluster = findNextCluster(fat16File->currentCluster);
                sectorIndexInCluster = 0;
            }
        }

        // Update the file's position
        fat16File->position += bytesRead;

        // Return the number of bytes read
        return bytesRead;
    }

    ssize_t FAT16FileSystem::write(File* file, const void* buffer, size_t size)
    {
        if (file == nullptr || buffer == nullptr || size == 0) {
            return -1;
        }

        FAT16File* fat16File = static_cast<FAT16File*>(file);

        auto bytesToWrite = size;
        const uint8_t* src = static_cast<const uint8_t*>(buffer);

        auto clusterOffset = fat16File->position % (m_bytesPerSector * m_sector.Bpb.sectorsPerCluster);

        uint32_t sectorIndexInCluster = clusterOffset / m_bytesPerSector;

        uint32_t sectorOffset = clusterOffset % m_bytesPerSector;

        uint32_t bytesWritten = 0;

        logMsg(dbgLog, "writeFile: bytesToWrite:%u c_Cluster:%u ClusterOffset:%u\n",
            bytesToWrite, fat16File->currentCluster, clusterOffset);

        bool directoryEntryChanged = false;

        while (bytesWritten < bytesToWrite) {
            uint32_t lba = clusterToSector(fat16File->currentCluster) + sectorIndexInCluster;

            auto writeSize = 0;

            if (sectorOffset == 0) {
                writeSize = Utils::min((size_t)m_bytesPerSector, bytesToWrite - bytesWritten);

                m_device.write(lba, src, writeSize);
            } else {
                // Read the sector containing file data into the buffer

                uint8_t readBuffer[512];

                m_device.readSector(lba, readBuffer);

                writeSize = Utils::min((size_t)m_bytesPerSector - sectorOffset, bytesToWrite - bytesWritten);

                memcpy(readBuffer + sectorOffset, src, writeSize);

                m_device.writeSector(lba, readBuffer);

                sectorOffset = 0;
            }

            bytesWritten += writeSize;
            src += writeSize;

            if (bytesWritten >= bytesToWrite) {
                break;
            }

            sectorIndexInCluster++;
            if (sectorIndexInCluster >= m_sector.Bpb.sectorsPerCluster) {
                uint32_t nextCluster = findNextCluster(fat16File->currentCluster);

                if (nextCluster == 0xFFFF) {
                    // No next cluster, allocate a new one
                    nextCluster = findFreeCluster();

                    if (nextCluster == 0xFFFF) {
                        logMsg(errLog, "Failed to allocate new cluster!\n");
                        return -1;
                    }

                    fat16File->endCluster = nextCluster;
                    // Update the FAT to link the clusters
                    updateFAT(fat16File->currentCluster, nextCluster);
                    updateFAT(nextCluster, 0xFFFF);

                    directoryEntryChanged = true;

                    logMsg(dbgLog, "allocate new cluster: %u\n", nextCluster);
                }

                fat16File->currentCluster = nextCluster;

                sectorIndexInCluster = 0;

                logMsg(dbgLog, "moved to next cluster: %u\n", fat16File->currentCluster);
            }
        }

        fat16File->position += bytesWritten;

        if (fat16File->position > fat16File->size) {
            fat16File->size = fat16File->position;
            directoryEntryChanged = true;
        }

        if (directoryEntryChanged) {
            fat16File->dirEntry->fileSize = fat16File->size;
            updateDirectoryEntry(fat16File->fileIndex);
            logMsg(dbgLog, "Write updateDirectoryEntry:%u \n", fat16File->fileIndex);
        }

        return bytesWritten;
    }

    void FAT16FileSystem::formatFilename(const char* filename, char* formattedName) const
    {
        // Clear the formatted name buffer
        memset(formattedName, ' ', 12); // 8 + 1 + 3

        // Split the filename into name and extension
        const char* dot = strchr(filename, '.');
        size_t nameLength = (dot) ? (dot - filename) : strlen(filename);
        size_t extLength = (dot) ? strlen(dot + 1) : 0;

        // Copy the name part (up to 8 characters)
        for (size_t i = 0; i < nameLength && i < 8; ++i) {
            formattedName[i] = static_cast<char>(toupper(filename[i])); // Convert to uppercase
        }

        // If there's an extension, copy it (up to 3 characters)
        if (dot) {
            for (size_t i = 0; i < extLength && i < 3; ++i) {
                formattedName[8 + i] = static_cast<char>(toupper(dot[i + 1])); // Convert to uppercase
            }
        }

        // Ensure the name is null-terminated (optional, as we use fixed size)
        formattedName[11] = '\0'; // Optional, for safety
    }

    uint32_t FAT16FileSystem::findNextCluster(uint32_t cluster) const
    {
        uint16_t fatTable[256] {};

        uint32_t fatSector = m_firstFatSector + cluster / 256;

        uint32_t fatOffset = cluster % 256;

        m_device.readSector(fatSector, (uint8_t*)fatTable);

        auto nextCluster = fatTable[fatOffset];

        if (nextCluster >= 0xFFF8) {
            nextCluster = 0xFFFF;
        }

        return nextCluster;
    }

    uint32_t FAT16FileSystem::clusterToSector(uint32_t cluster) const
    {
        return m_firstDataSector + (cluster - 2) * m_sector.Bpb.sectorsPerCluster;
    }

    uint16_t FAT16FileSystem::findFreeCluster() const
    {
        uint16_t fatTable[256] {};

        for (uint32_t i = 0; i < m_fatSize; ++i) {
            m_device.readSector(m_firstFatSector + i, (uint8_t*)fatTable);

            for (int j = 0; j < 256; ++j) {
                if (fatTable[j] == 0x0000) {
                    return (i * 256) + j;
                }
            }
        }

        return 0xFFFF;
    }

    void FAT16FileSystem::updateFAT(uint16_t cluster, uint16_t nextCluster)
    {
        uint16_t fatTable[256] {};

        uint32_t fatSector = m_firstFatSector + cluster / 256;

        uint32_t fatOffset = cluster % 256;

        for (auto i = 0; i < m_sector.Bpb.numberOfFats; i++) {
            m_device.readSector(fatSector, (uint8_t*)fatTable);
            fatTable[fatOffset] = nextCluster;

            m_device.writeSector(fatSector, (uint8_t*)fatTable);
            fatSector += m_fatSize;
        }
    }

    void FAT16FileSystem::updateDirectoryEntry(size_t entryIndex)
    {

        uint32_t sector = m_firstRootDirSector + (entryIndex / (m_bytesPerSector / sizeof(Directory)));
        uint32_t offset = (entryIndex % (m_bytesPerSector / sizeof(Directory))) * sizeof(Directory);

        uint8_t buffer[512];
        m_device.readSector(sector, buffer);
        memcpy(&buffer[offset], &m_dirEntry[entryIndex], sizeof(Directory));
        m_device.writeSector(sector, buffer);
    }

}; // namespace FAT16
} // namespace FileSystem