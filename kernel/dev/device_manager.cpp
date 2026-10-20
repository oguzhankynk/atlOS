/**
 * @file device_manager.cpp
 * @author Oguzhan KAYNAK ()
 * @brief
 *
 * Copyright © 2024 Oguzhan KAYNAK
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software and
 * associated documentation files (the “Software”), to deal in the Software without  restriction,
 * including without limitation the rights to use, copy, modify, merge, publish, distribute,
 * sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or
 * substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT
 * NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <dev/device_manager.hpp>

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <drivers/disk/ata.hpp>
#include <drivers/keyboard.hpp>

#include <arch/x86_64/idt/handlers.hpp>

#include <fs/fat16.hpp>
#include <fs/filesystem.hpp>
#include <fs/vfs.hpp>

#include <core/panic.hpp>

namespace DeviceManager {
static constexpr size_t numberOfMaxDevice = 10;

static Device* s_deviceList[numberOfMaxDevice] {};

struct PartitionEntry {
    uint8_t status {}; // Partition status
    uint8_t chsFirst[3] {}; // CHS address of first absolute sector
    uint8_t type {}; // Partition type
    uint8_t chsLast[3] {}; // CHS address of last absolute sector
    uint32_t lbaFirstSector {}; // LBA of first absolute sector
    uint32_t numSectors {}; // Number of sectors in partition

    void print() const noexcept
    {
        printf("  Status: 0x%02X ", status);
        printf("  Type: 0x%02X ", type);
        printf("  First LBA sector: %u ", lbaFirstSector);
        printf("  Number of sectors: %u\n", numSectors);
    }

} __attribute__((packed));

/**
 * @brief Master Boot Record.
 *
 */
struct MBR {
    uint8_t code[446] {}; /// Code Area
    PartitionEntry partitionTable[4] {}; /// Partition tables
    uint16_t magicNumber {}; /// MBR signature :  0xAA55.
} __attribute__((packed));

static void detectATADevices()
{
    ATA* ataDevice = new ATA("/dev/ata0");

    kernelPanicAssert(ataDevice != nullptr);

    uint8_t sectorData[512] {};

    //! read boot sector
    ataDevice->readSector(0, (uint8_t*)sectorData);

    if (sectorData[510] == 0x55 && sectorData[511] == 0xAA) {
        if ((sectorData[446] == 0x00 || sectorData[446] == 0x80) && sectorData[450] != 0x00) {
            // MBR detected

            const MBR& mbr = *(MBR*)sectorData;

            auto validPartition = 0;

            printf("MBR info: \n");
            for (const auto& part : mbr.partitionTable) {
                if (part.type != 0x00) {
                    part.print();

                    char deviceName[32];
                    snprintf(deviceName, 32, "/dev/ata0p%d", validPartition);

                    ATA* newPartition = new ATA(deviceName);
                    kernelPanicAssert(newPartition != nullptr);

                    uint8_t partitionSectorData[512] {};

                    newPartition->readSector(part.lbaFirstSector, partitionSectorData);

                    if (partitionSectorData[0] == 0xEB || partitionSectorData[0] == 0xE9) {
                        // FAT 16 detected

                        auto* fat16 = new FileSystem::FAT16::FAT16FileSystem(*newPartition,
                            part.lbaFirstSector);
                        if (!FileSystem::VFS::addFileSystem(*fat16))
                            printf("DeviceManager: no free VFS slot for the FAT16 partition\n");

                        break;
                    }
                    // TODO

                    validPartition++;
                }
            }

            for (const auto& part : mbr.partitionTable) {
            }
        } else {
            // TODO
        }
    } else {
        printf("Invalid disk signature\n");
        delete ataDevice;
        return;
    }
}

void init()
{

    // detectATADevices();
}
} // namespace DeviceManager