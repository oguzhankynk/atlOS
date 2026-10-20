/**
 * @file memory_block_device.hpp
 * @author Oguzhan KAYNAK
 * @brief Drivers::MemoryBlockDevice: a block device over a buffer in RAM, for
 * testing file systems without a disk.
 */

#pragma once

#include <drivers/disk/block_device.hpp>

namespace Drivers {

class MemoryBlockDevice final : public BlockDevice {
public:
    /// The buffer holds sectorCount * sectorSize bytes and stays owned by the caller.
    MemoryBlockDevice(void* buffer, uint64_t sectorCount);

    bool readSectors(uint64_t lba, uint32_t count, void* buffer) override;
    bool writeSectors(uint64_t lba, uint32_t count, const void* buffer) override;
    uint64_t sectorCount() const override { return m_sectorCount; }
    const char* name() const override { return "ram"; }

private:
    uint8_t* m_data;
    uint64_t m_sectorCount;
};

} // namespace Drivers
