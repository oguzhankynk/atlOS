/**
 * @file partition.hpp
 * @author Oguzhan KAYNAK
 * @brief MBR partition table, and a partition as a block device of its own.
 */

#pragma once

#include <stdint.h>

#include <drivers/disk/block_device.hpp>

namespace Drivers {

/// @brief One used entry of the MBR partition table.
struct PartitionInfo {
    uint8_t type; ///< e.g. 0x04, 0x06, 0x0E: FAT16
    uint64_t firstLba;
    uint64_t sectorCount;
};

static constexpr int mbrPartitionCount = 4;

/**
 * @brief Read the MBR (sector 0) of disk and copy its used entries to partitions.
 * @return The number of used entries (0..4), -1 if sector 0 is not an MBR
 * (no 0x55AA signature, or an entry that does not fit on the disk).
 */
int readPartitionTable(BlockDevice& disk, PartitionInfo partitions[mbrPartitionCount]);

/// @brief Whether an MBR partition type is one of the FAT16 types.
bool isFat16PartitionType(uint8_t type);

/**
 * @brief A range of sectors of another device, seen as a device starting at sector 0.
 */
class PartitionDevice final : public BlockDevice {
public:
    /// name is copied (up to 15 characters).
    PartitionDevice(BlockDevice& disk, uint64_t firstLba, uint64_t sectorCount, const char* name);

    bool readSectors(uint64_t lba, uint32_t count, void* buffer) override;
    bool writeSectors(uint64_t lba, uint32_t count, const void* buffer) override;
    uint64_t sectorCount() const override { return m_sectorCount; }
    const char* name() const override { return m_name; }

private:
    BlockDevice& m_disk;
    uint64_t m_firstLba;
    uint64_t m_sectorCount;
    char m_name[16] {};
};

} // namespace Drivers
