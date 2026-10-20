/**
 * @file block_device.hpp
 * @author Oguzhan KAYNAK
 * @brief Interface of a device that stores fixed size sectors (a disk, a
 * partition of it, a disk image in RAM).
 */

#pragma once

#include <stdint.h>

namespace Drivers {

class BlockDevice {
public:
    static constexpr uint32_t sectorSize = 512;

    /**
     * @brief Read count sectors starting at lba into buffer (count * sectorSize bytes).
     * @return false on a device error or a range beyond sectorCount().
     */
    virtual bool readSectors(uint64_t lba, uint32_t count, void* buffer) = 0;

    /// Write count sectors starting at lba from buffer.
    virtual bool writeSectors(uint64_t lba, uint32_t count, const void* buffer) = 0;

    virtual uint64_t sectorCount() const = 0;

    /// A short name for messages, e.g. "ata0" or "ata0p1".
    virtual const char* name() const = 0;

protected:
    // Never deleted through the interface
    ~BlockDevice() = default;
};

/// @brief Whether [lba, lba + count) lies inside a device of sectorCount sectors.
inline bool isSectorRangeValid(uint64_t lba, uint32_t count, uint64_t sectorCount)
{
    return lba <= sectorCount && count <= sectorCount - lba;
}

} // namespace Drivers
