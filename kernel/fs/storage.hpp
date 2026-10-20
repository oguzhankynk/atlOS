/**
 * @file storage.hpp
 * @author Oguzhan KAYNAK
 * @brief Find the ATA disks and mount their FAT16 partitions.
 */

#pragma once

#include <fs/vfs.hpp>

namespace Fs {

/**
 * @brief Probe the four ATA drives (primary/secondary, master/slave), read
 * each disk's MBR and mount every FAT16 partition as /disk0, /disk1, ...
 * (a disk without a partition table is tried as one FAT16 volume).
 *
 * The device and file system objects are created on the heap and live as
 * long as the kernel. Prints what it finds.
 *
 * @return The number of file systems mounted.
 */
int mountAtaDisks(Vfs& vfs);

} // namespace Fs
