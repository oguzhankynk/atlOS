/**
 * @file DeviceManager.cpp
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

#include <Device/DeviceManager.hpp>

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <hardware/pic.hpp>
#include <hardware/pit.hpp>
#include <Interfaces/serial.hpp>
#include <Interfaces/Disk/ATA.hpp>
#include <Drivers/keyboard.hpp>

#include <kernel/sys/handlers.hpp>

#include <FileSystem/FileSystem.hpp>
#include <FileSystem/VFS.hpp>
#include <FileSystem/FAT16.hpp>

#include <kernel/panic.h>

namespace DeviceManager
{
    static constexpr size_t NUMBER_OF_MAX_DEVICE = 10;

    static Device *deviceList[NUMBER_OF_MAX_DEVICE]{};

    struct PartitionEntry
    {
        uint8_t status{};            // Partition status
        uint8_t chs_first[3]{};      // CHS address of first absolute sector
        uint8_t type{};              // Partition type
        uint8_t chs_last[3]{};       // CHS address of last absolute sector
        uint32_t lba_first_sector{}; // LBA of first absolute sector
        uint32_t num_sectors{};      // Number of sectors in partition

        void print() const noexcept
        {
            printf("  Status: 0x%02X ", status);
            printf("  Type: 0x%02X ", type);
            printf("  First LBA sector: %u ", lba_first_sector);
            printf("  Number of sectors: %u\n", num_sectors);
        }

    } __attribute__((packed));

    /**
     * @brief Master Boot Record.
     *
     */
    struct MBR
    {
        uint8_t code[446]{};                 /// Code Area
        PartitionEntry partition_table[4]{}; /// Partition tables
        uint16_t magic_number{};             /// MBR signature :  0xAA55.
    } __attribute__((packed));

    static void detectATADevices()
    {
        ATA *ataDevice = new ATA("/dev/ata0");

        kernel_panic_assert(ataDevice != nullptr);

        uint8_t sectorData[512]{};

        //! read boot sector
        ataDevice->read_sector(0, (uint8_t *)sectorData);

        if (sectorData[510] == 0x55 && sectorData[511] == 0xAA)
        {
            if ((sectorData[446] == 0x00 || sectorData[446] == 0x80) && sectorData[450] != 0x00)
            {
                // MBR detected

                const MBR &mbr = *(MBR *)sectorData;

                auto validPartition = 0;

                printf("MBR info: \n");
                for (const auto &part : mbr.partition_table)
                {
                    if (part.type != 0x00)
                    {
                        part.print();

                        char deviceName[32];
                        snprintf(deviceName, 32, "/dev/ata0p%d", validPartition);

                        ATA *newPartition = new ATA(deviceName);
                        kernel_panic_assert(newPartition != nullptr);

                        uint8_t partitionSectorData[512]{};

                        newPartition->read_sector(part.lba_first_sector, partitionSectorData);

                        if (partitionSectorData[0] == 0xEB || partitionSectorData[0] == 0xE9)
                        {
                            // FAT 16 detected

                            auto *fat16 = new FileSystem::FAT16::FAT16FileSystem(*newPartition,
                                                                                 part.lba_first_sector);
                            auto ret = FileSystem::VFS::AddFileSystem(*fat16);

                            break;
                        }
                        // TODO

                        validPartition++;
                    }
                }

                for (const auto &part : mbr.partition_table)
                {
                }
            }
            else
            {
                // TODO
            }
        }
        else
        {
            printf("Invalid disk signature\n");
            delete ataDevice;
            return;
        }
    }

    void Init()
    {
        PIC::Init();

        // Keyboard::Init();

        int stat_serial_init = SerialPort::Init();

        printf("stat_serial_init: %d \n", stat_serial_init);

        detectATADevices();

        PIT::init(100000000000); // 1ms
    }
} // namespace DeviceManager