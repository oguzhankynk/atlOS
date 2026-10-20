/**
 * @file storage.cpp
 * @author Oguzhan KAYNAK
 * @brief Find the ATA disks and mount their FAT16 partitions.
 */

#include <fs/storage.hpp>

#include <stdio.h>

#include <drivers/disk/ata.hpp>
#include <drivers/disk/partition.hpp>
#include <fs/fat16.hpp>

namespace Fs {

using Drivers::ATAPioDevice;

static bool mountFat16(Vfs& vfs, Drivers::BlockDevice& volume, int& mounted)
{
    auto* fat = new Fat16(volume);
    if (!fat->mount()) {
        delete fat;
        return false;
    }

    char name[Vfs::maxMountNameLength + 1];
    snprintf(name, sizeof(name), "disk%d", mounted);
    if (!vfs.mount(name, *fat)) {
        delete fat;
        return false;
    }

    printf("storage: %s: fat16, %u clusters of %u bytes, mounted on /%s\n", volume.name(), fat->clusterCount(),
        fat->clusterSize(), name);
    mounted++;
    return true;
}

static void mountPartitions(Vfs& vfs, ATAPioDevice& disk, int& mounted)
{
    Drivers::PartitionInfo partitions[Drivers::mbrPartitionCount];
    int count = Drivers::readPartitionTable(disk, partitions);

    if (count < 0) {
        // No partition table: maybe the whole disk is one volume
        if (!mountFat16(vfs, disk, mounted))
            printf("storage: %s: no partition table and no FAT16 volume\n", disk.name());
        return;
    }

    for (int i = 0; i < count; i++) {
        const Drivers::PartitionInfo& part = partitions[i];
        if (!Drivers::isFat16PartitionType(part.type)) {
            printf("storage: %sp%d: type 0x%02x, not FAT16, skipped\n", disk.name(), i + 1, part.type);
            continue;
        }

        char name[16];
        snprintf(name, sizeof(name), "%sp%d", disk.name(), i + 1);
        auto* partition = new Drivers::PartitionDevice(disk, part.firstLba, part.sectorCount, name);
        if (!mountFat16(vfs, *partition, mounted)) {
            printf("storage: %s: FAT16 partition type, but no valid FAT16 volume\n", name);
            delete partition;
        }
    }
}

int mountAtaDisks(Vfs& vfs)
{
    static const struct {
        ATAPioDevice::Channel channel;
        ATAPioDevice::Drive drive;
    } s_drives[] = {
        { ATAPioDevice::Channel::Primary, ATAPioDevice::Drive::Master },
        { ATAPioDevice::Channel::Primary, ATAPioDevice::Drive::Slave },
        { ATAPioDevice::Channel::Secondary, ATAPioDevice::Drive::Master },
        { ATAPioDevice::Channel::Secondary, ATAPioDevice::Drive::Slave },
    };

    int mounted = 0;
    for (uint32_t i = 0; i < sizeof(s_drives) / sizeof(s_drives[0]); i++) {
        char name[16];
        snprintf(name, sizeof(name), "ata%u", i);
        auto* disk = new ATAPioDevice(s_drives[i].channel, s_drives[i].drive, name);
        if (!disk->isPresent()) {
            delete disk;
            continue;
        }

        printf("storage: %s: \"%s\", %lu sectors (%lu MB)\n", name, disk->model(), disk->sectorCount(),
            disk->sectorCount() / 2048);
        mountPartitions(vfs, *disk, mounted);
    }
    return mounted;
}

} // namespace Fs
