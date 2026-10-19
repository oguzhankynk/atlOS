/**
 * @file ata.cpp
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

#include <drivers/disk/ata.hpp>
#include <arch/x86_64/io/ioport.hpp>

#include <string.h>

static constexpr uint32_t ataPrimaryIoBase = 0x1F0;
static constexpr uint32_t ataPrimaryCtrlBase = 0x3F6;

static constexpr uint32_t ataRegData = 0x00;
static constexpr uint32_t ataRegError = 0x01;
static constexpr uint32_t ataRegSeccount = 0x02;
static constexpr uint32_t ataRegLbaLow = 0x03;
static constexpr uint32_t ataRegLbaMid = 0x04;
static constexpr uint32_t ataRegLbaHigh = 0x05;
static constexpr uint32_t ataRegDrive = 0x06;
static constexpr uint32_t ataRegStatus = 0x07;
static constexpr uint32_t ataRegCommand = 0x07;

static constexpr uint32_t ataCmdReadPio = 0x20;
static constexpr uint32_t ataCmdWritePio = 0x30;

static constexpr uint32_t ataStatusErr = 1U << 0;
static constexpr uint32_t ataStatusIdx = 1U << 1;
static constexpr uint32_t ataStatusCorr = 1U << 2;
static constexpr uint32_t ataStatusDrq = 1U << 3;
static constexpr uint32_t ataStatusSrv = 1U << 4;
static constexpr uint32_t ataStatusDf = 1U << 5;
static constexpr uint32_t ataStatusRdy = 1U << 6;
static constexpr uint32_t ataStatusBsy = 1U << 7;

static inline void ataWaitBsy()
{
    while (IOPort::inportb(ataPrimaryIoBase + ataRegStatus) & ataStatusBsy)
        ;
}

static inline void ataWaitDrq()
{
    while (!(IOPort::inportb(ataPrimaryIoBase + ataRegStatus) & ataStatusDrq))
        ;
}

static inline void delay400ns()
{
    IOPort::inportb(ataPrimaryIoBase + ataRegStatus);
    IOPort::inportb(ataPrimaryIoBase + ataRegStatus);
    IOPort::inportb(ataPrimaryIoBase + ataRegStatus);
    IOPort::inportb(ataPrimaryIoBase + ataRegStatus);
}

ATA::ATA(const char *name) noexcept
    : Disk(name)
{
}

/**
 * @brief Reads a single sector from the disk into the buffer.
 *
 * @param lba The Logical Block Address (LBA) of the sector to read.
 * @param buffer Pointer to the buffer where the data will be stored.
 */
void ATA::readSector(uint32_t lba, uint8_t *buffer) const
{
    ataWaitBsy();

    IOPort::outportb(ataPrimaryIoBase + ataRegDrive, 0xE0 | ((lba >> 24) & 0x0F));
    IOPort::outportb(ataPrimaryIoBase + ataRegSeccount, 1);
    IOPort::outportb(ataPrimaryIoBase + ataRegLbaLow, lba & 0xFF);
    IOPort::outportb(ataPrimaryIoBase + ataRegLbaMid, (lba >> 8) & 0xFF);
    IOPort::outportb(ataPrimaryIoBase + ataRegLbaHigh, (lba >> 16) & 0xFF);
    IOPort::outportb(ataPrimaryIoBase + ataRegCommand, ataCmdReadPio);

    delay400ns();

    ataWaitBsy();
    ataWaitDrq();

    for (int i = 0; i < 256; i++)
    {
        ((uint16_t *)buffer)[i] = IOPort::inportw(ataPrimaryIoBase + ataRegData);
    }

    ataWaitBsy();
}

/**
 * @brief Reads multiple sectors from the disk into the buffer.
 *
 * @param lbaStart The starting LBA of the first sector to read.
 * @param buffer Pointer to the buffer where the data will be stored.
 * @param size The size of data to read in bytes.
 */
void ATA::read(uint32_t lbaStart, uint8_t *buffer, size_t size) const
{
    auto step = 0;

    while (size >= 512)
    {
        readSector(lbaStart + step, buffer + 512 * step);
        step++;
        size -= 512;
    }

    if (size > 0)
    {
        uint8_t data[512]{};
        readSector(lbaStart + step, data);
        memcpy(buffer + 512 * step, data, size);
    }
}

/**
 * @brief Writes a single sector to the disk from the buffer.
 *
 * @param lba The Logical Block Address (LBA) of the sector to write.
 * @param buffer Pointer to the buffer containing the data to write.
 */
void ATA::writeSector(uint32_t lba, const uint8_t *buffer) const
{
    ataWaitBsy();

    IOPort::outportb(ataPrimaryIoBase + ataRegDrive, 0xE0 | ((lba >> 24) & 0x0F));
    IOPort::outportb(ataPrimaryIoBase + ataRegSeccount, 1);
    IOPort::outportb(ataPrimaryIoBase + ataRegLbaLow, lba & 0xFF);
    IOPort::outportb(ataPrimaryIoBase + ataRegLbaMid, (lba >> 8) & 0xFF);
    IOPort::outportb(ataPrimaryIoBase + ataRegLbaHigh, (lba >> 16) & 0xFF);
    IOPort::outportb(ataPrimaryIoBase + ataRegCommand, ataCmdWritePio);

    ataWaitBsy();
    ataWaitDrq();

    for (int i = 0; i < 256; i++)
    {
        IOPort::outportw(ataPrimaryIoBase + ataRegData, ((uint16_t *)buffer)[i]);
    }

    ataWaitBsy();
}

/**
 * @brief Writes multiple sectors to the disk from the buffer.
 *
 * @param lbaStart The starting LBA of the first sector to write.
 * @param buffer Pointer to the buffer containing the data to write.
 * @param size The size of data to write in bytes.
 */
void ATA::write(uint32_t lbaStart, const uint8_t *buffer, size_t size) const
{
    auto step = 0;

    while (size >= 512)
    {
        writeSector(lbaStart + step, buffer + 512 * step);
        step++;
        size -= 512;
    }

    if (size > 0)
    {
        uint8_t data[512]{};
        memcpy(data, buffer + 512 * step, size);
        writeSector(lbaStart + step, data);
    }
}
