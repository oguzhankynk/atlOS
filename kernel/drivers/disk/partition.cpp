/**
 * @file partition.cpp
 * @author Oguzhan KAYNAK
 * @brief MBR partition table, and a partition as a block device of its own.
 */

#include <drivers/disk/partition.hpp>

#include <string.h>

namespace Drivers {

// Sector 0 of an MBR disk: 446 bytes of boot code, four 16 byte entries, 0x55 0xAA
struct MbrEntry {
    uint8_t status; // 0x80: active (boot), 0x00: not
    uint8_t chsFirst[3]; // CHS addresses: obsolete, LBA is used
    uint8_t type; // 0x00: unused
    uint8_t chsLast[3];
    uint32_t firstLba;
    uint32_t sectorCount;
} __attribute__((packed));

struct Mbr {
    uint8_t bootCode[446];
    MbrEntry entries[mbrPartitionCount];
    uint8_t signature[2];
} __attribute__((packed));

static_assert(sizeof(Mbr) == BlockDevice::sectorSize, "the MBR is one sector");

int readPartitionTable(BlockDevice& disk, PartitionInfo partitions[mbrPartitionCount])
{
    Mbr mbr;
    if (!disk.readSectors(0, 1, &mbr))
        return -1;

    if (mbr.signature[0] != 0x55 || mbr.signature[1] != 0xAA)
        return -1;

    int count = 0;
    for (const MbrEntry& entry : mbr.entries) {
        if (!entry.type || !entry.sectorCount)
            continue;

        // A boot sector of a disk without partitions (a "superfloppy") also
        // ends in 0x55AA: its "entries" are boot code and rarely fit the disk
        if (!isSectorRangeValid(entry.firstLba, entry.sectorCount, disk.sectorCount()) || !entry.firstLba)
            return -1;

        partitions[count++] = { entry.type, entry.firstLba, entry.sectorCount };
    }
    return count;
}

bool isFat16PartitionType(uint8_t type)
{
    // 0x04: FAT16 < 32 MB, 0x06: FAT16, 0x0E: FAT16 with LBA
    return type == 0x04 || type == 0x06 || type == 0x0E;
}

PartitionDevice::PartitionDevice(BlockDevice& disk, uint64_t firstLba, uint64_t sectorCount, const char* name)
    : m_disk(disk)
    , m_firstLba(firstLba)
    , m_sectorCount(sectorCount)
{
    strncpy(m_name, name, sizeof(m_name) - 1);
}

bool PartitionDevice::readSectors(uint64_t lba, uint32_t count, void* buffer)
{
    if (!isSectorRangeValid(lba, count, m_sectorCount))
        return false;
    return m_disk.readSectors(m_firstLba + lba, count, buffer);
}

bool PartitionDevice::writeSectors(uint64_t lba, uint32_t count, const void* buffer)
{
    if (!isSectorRangeValid(lba, count, m_sectorCount))
        return false;
    return m_disk.writeSectors(m_firstLba + lba, count, buffer);
}

} // namespace Drivers
