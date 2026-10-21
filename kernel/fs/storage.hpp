/**
 * @file storage.hpp
 * @author Oguzhan KAYNAK
 * @brief Find the ATA disks and mount their FAT16 partitions.
 */

#pragma once

#include <stdint.h>

#include <drivers/disk/block_device.hpp>
#include <fs/vfs.hpp>

namespace Fs {

/// The disks and partitions found at boot, by name (the shell's disk command).
struct BlockDeviceList {
    static constexpr uint32_t capacity = 16;

    Drivers::BlockDevice* devices[capacity] {};
    uint32_t count {};

    /// false when full
    bool add(Drivers::BlockDevice& device);

    /// nullptr if there is none of that name
    Drivers::BlockDevice* find(const char* name) const;
};

/**
 * @brief Probe the four ATA drives (primary/secondary, master/slave), read
 * each disk's MBR and mount every FAT16 partition as /disk0, /disk1, ...
 * (a disk without a partition table is tried as one FAT16 volume).
 *
 * The device and file system objects are created on the heap and live as
 * long as the kernel; every disk and partition also goes into devices.
 * Prints what it finds.
 *
 * @return The number of file systems mounted.
 */
int mountAtaDisks(Vfs& vfs, BlockDeviceList& devices);

} // namespace Fs
