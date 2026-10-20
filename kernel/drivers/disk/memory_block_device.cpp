/**
 * @file memory_block_device.cpp
 * @author Oguzhan KAYNAK
 * @brief Drivers::MemoryBlockDevice: a block device over a buffer in RAM.
 */

#include <drivers/disk/memory_block_device.hpp>

#include <string.h>

namespace Drivers {

MemoryBlockDevice::MemoryBlockDevice(void* buffer, uint64_t sectorCount)
    : m_data(static_cast<uint8_t*>(buffer))
    , m_sectorCount(sectorCount)
{
}

bool MemoryBlockDevice::readSectors(uint64_t lba, uint32_t count, void* buffer)
{
    if (!isSectorRangeValid(lba, count, m_sectorCount))
        return false;
    memcpy(buffer, m_data + lba * sectorSize, (size_t)count * sectorSize);
    return true;
}

bool MemoryBlockDevice::writeSectors(uint64_t lba, uint32_t count, const void* buffer)
{
    if (!isSectorRangeValid(lba, count, m_sectorCount))
        return false;
    memcpy(m_data + lba * sectorSize, buffer, (size_t)count * sectorSize);
    return true;
}

} // namespace Drivers
