/**
 * @file ATA.cpp
 * @author Oguzhan KAYNAK
 * @brief Implements the ATA class that implements disk operations using the ATA protocol.
 *
 * Copyright © 2024 Oguzhan KAYNAK
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the “Software”), to deal in the Software
 * without  restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit
 * persons to whom the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
 * PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <Interfaces/Disk/ATA.hpp>
#include <kernel/sys/IOPort.hpp>

#include <string.h>



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

static inline void ata_wait_bsy()
{
    while (IOPort::inportb(ATA_PRIMARY_IO_BASE + ATA_REG_STATUS) & ATA_STATUS_BSY);
}

static inline void ata_wait_drq()
{
    while (!(IOPort::inportb(ATA_PRIMARY_IO_BASE + ATA_REG_STATUS) & ATA_STATUS_DRQ));
}

static inline void delay_400ns()
{
    IOPort::inportb(ATA_PRIMARY_IO_BASE + ATA_REG_STATUS);
    IOPort::inportb(ATA_PRIMARY_IO_BASE + ATA_REG_STATUS);
    IOPort::inportb(ATA_PRIMARY_IO_BASE + ATA_REG_STATUS);
    IOPort::inportb(ATA_PRIMARY_IO_BASE + ATA_REG_STATUS);
}

ATA::ATA(const char* name) noexcept
:Disk(name)
{
    
}

/**
 * @brief Reads a single sector from the disk into the buffer.
 *
 * @param lba The Logical Block Address (LBA) of the sector to read.
 * @param buffer Pointer to the buffer where the data will be stored.
 */
void ATA::read_sector(uint32_t lba, uint8_t* buffer) const
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

/**
 * @brief Reads multiple sectors from the disk into the buffer.
 *
 * @param lba_start The starting LBA of the first sector to read.
 * @param buffer Pointer to the buffer where the data will be stored.
 * @param size The size of data to read in bytes.
 */
void ATA::read(uint32_t lba_start, uint8_t* buffer, size_t size) const
{
    auto step = 0;

    while (size >= 512)
    {
        read_sector(lba_start + step, buffer + 512 * step);
        step++;
        size -= 512;
    }

    if (size > 0)
    {
        uint8_t data[512]{};
        read_sector(lba_start + step, data);
        memcpy(buffer + 512 * step, data, size);
    }
}

/**
 * @brief Writes a single sector to the disk from the buffer.
 *
 * @param lba The Logical Block Address (LBA) of the sector to write.
 * @param buffer Pointer to the buffer containing the data to write.
 */
void ATA::write_sector(uint32_t lba, const uint8_t* buffer) const
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

/**
 * @brief Writes multiple sectors to the disk from the buffer.
 *
 * @param lba_start The starting LBA of the first sector to write.
 * @param buffer Pointer to the buffer containing the data to write.
 * @param size The size of data to write in bytes.
 */
void ATA::write(uint32_t lba_start, const uint8_t* buffer, size_t size) const
{
    auto step = 0;

    while (size >= 512)
    {
        write_sector(lba_start + step, buffer + 512 * step);
        step++;
        size -= 512;
    }

    if (size > 0)
    {
        uint8_t data[512]{};
        memcpy(data, buffer + 512 * step, size);
        write_sector(lba_start + step, data);
    }
}

