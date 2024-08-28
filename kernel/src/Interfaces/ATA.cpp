/**
 * @file ATA.cpp
 * @author Oguzhan KAYNAK
 * @brief
 *
 * Copyright © 2024 Oguzhan KAYNAK
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the “Software”), to deal in the Software *without  restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit *persons to whom the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR *PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR *OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include <Interfaces/ATA.hpp>
#include <kernel/sys/IOPort.hpp>

namespace ATA
{

    static constexpr uint32_t ATA_PRIMARY_IO_BASE = 0x1F0;
    static constexpr uint32_t ATA_PRIMARY_CTRL_BASE = 0x3F6;

    static constexpr uint32_t ATA_REG_DATA = 0x00;
    static constexpr uint32_t ATA_REG_ERROR = 0x01;
    static constexpr uint32_t ATA_REG_SECCOUNT = 0x02;
    static constexpr uint32_t ATA_REG_LBA_LOW = 0x03;
    static constexpr uint32_t ATA_REG_LBA_MID = 0x04;
    static constexpr uint32_t ATA_REG_LBA_HIGH = 0x05;
    static constexpr uint32_t ATA_REG_DRIVE = 0x06;
    static constexpr uint32_t ATA_REG_STATUS = 0x07;
    static constexpr uint32_t ATA_REG_COMMAND = 0x07;

    static constexpr uint32_t ATA_CMD_READ_PIO = 0x20;
    static constexpr uint32_t ATA_CMD_WRITE_PIO = 0x30;

    static constexpr uint32_t ATA_STATUS_ERR = 1U << 0;
    static constexpr uint32_t ATA_STATUS_IDX = 1U << 1;
    static constexpr uint32_t ATA_STATUS_CORR = 1U << 2;
    static constexpr uint32_t ATA_STATUS_DRQ = 1U << 3;
    static constexpr uint32_t ATA_STATUS_SRV = 1U << 4;
    static constexpr uint32_t ATA_STATUS_DF = 1U << 5;
    static constexpr uint32_t ATA_STATUS_RDY = 1U << 6;
    static constexpr uint32_t ATA_STATUS_BSY = 1U << 7;

    static void ata_wait_bsy();
    static void ata_wait_drq();
    static void delay_400ns();

    void ata_read_sector(uint32_t lba, uint8_t* buffer)
    {
        ata_wait_bsy();


        IOPort::outportb(ATA_PRIMARY_IO_BASE + ATA_REG_DRIVE, 0xE0 | ((lba >> 24) & 0x0F));
        IOPort::outportb(ATA_PRIMARY_IO_BASE + ATA_REG_SECCOUNT, 1);
        IOPort::outportb(ATA_PRIMARY_IO_BASE + ATA_REG_LBA_LOW, lba & 0xFF);
        IOPort::outportb(ATA_PRIMARY_IO_BASE + ATA_REG_LBA_MID, (lba >> 8) & 0xFF);
        IOPort::outportb(ATA_PRIMARY_IO_BASE + ATA_REG_LBA_HIGH, (lba >> 16) & 0xFF);
        IOPort::outportb(ATA_PRIMARY_IO_BASE + ATA_REG_COMMAND, ATA_CMD_READ_PIO);

        delay_400ns();
        
        ata_wait_bsy();
        ata_wait_drq();

        for (int i = 0; i < 256; i++) {
            ((uint16_t*)buffer)[i] = IOPort::inportw(ATA_PRIMARY_IO_BASE + ATA_REG_DATA);
        }

        ata_wait_bsy();
    }

    void ata_write_sector(uint32_t lba, uint8_t* buffer)
    {
        ata_wait_bsy();


        IOPort::outportb(ATA_PRIMARY_IO_BASE + ATA_REG_DRIVE, 0xE0 | ((lba >> 24) & 0x0F));
        IOPort::outportb(ATA_PRIMARY_IO_BASE + ATA_REG_SECCOUNT, 1);
        IOPort::outportb(ATA_PRIMARY_IO_BASE + ATA_REG_LBA_LOW, lba & 0xFF);
        IOPort::outportb(ATA_PRIMARY_IO_BASE + ATA_REG_LBA_MID, (lba >> 8) & 0xFF);
        IOPort::outportb(ATA_PRIMARY_IO_BASE + ATA_REG_LBA_HIGH, (lba >> 16) & 0xFF);
        IOPort::outportb(ATA_PRIMARY_IO_BASE + ATA_REG_COMMAND, ATA_CMD_WRITE_PIO);
        
        ata_wait_bsy();
        ata_wait_drq();


        for (int i = 0; i < 256; i++) {
            IOPort::outportw(ATA_PRIMARY_IO_BASE + ATA_REG_DATA, ((uint16_t*)buffer)[i]);
        }

        ata_wait_bsy();
    }


    static void ata_wait_bsy()
    {
        while (IOPort::inportb(ATA_PRIMARY_IO_BASE + ATA_REG_STATUS) & ATA_STATUS_BSY);
    }

    static void ata_wait_drq()
    {
        while (!(IOPort::inportb(ATA_PRIMARY_IO_BASE + ATA_REG_STATUS) & ATA_STATUS_DRQ));
    }

    static void delay_400ns()
    {
        IOPort::inportb(ATA_PRIMARY_IO_BASE + ATA_REG_STATUS);
        IOPort::inportb(ATA_PRIMARY_IO_BASE + ATA_REG_STATUS);
        IOPort::inportb(ATA_PRIMARY_IO_BASE + ATA_REG_STATUS);
        IOPort::inportb(ATA_PRIMARY_IO_BASE + ATA_REG_STATUS);
    }

};//namespace ATA

