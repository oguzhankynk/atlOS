/**
 * @file ata.cpp
 * @author Oguzhan KAYNAK
 * @brief Drivers::ATAPioDevice: an ATA (IDE) disk with programmed I/O.
 *
 * Only 28-bit LBA commands: up to 2^28 sectors (128 GB).
 */

#include <drivers/disk/ata.hpp>

#include <string.h>

#include <arch/x86_64/io/ioport.hpp>

namespace Drivers {

// Registers, as offsets from the channel's I/O base
static constexpr uint16_t regData = 0;
static constexpr uint16_t regSectorCount = 2;
static constexpr uint16_t regLba0 = 3;
static constexpr uint16_t regLba1 = 4;
static constexpr uint16_t regLba2 = 5;
static constexpr uint16_t regDrive = 6; // bits 0..3: LBA 24..27, bit 4: slave, 0xE0: LBA mode
static constexpr uint16_t regStatus = 7; // read
static constexpr uint16_t regCommand = 7; // write

// Device control register (at the control base)
static constexpr uint8_t controlNoInterrupt = 1 << 1; // nIEN

static constexpr uint8_t statusError = 1 << 0;
static constexpr uint8_t statusDataRequest = 1 << 3; // DRQ: the drive wants to transfer a sector
static constexpr uint8_t statusDeviceFault = 1 << 5;
static constexpr uint8_t statusBusy = 1 << 7;

static constexpr uint8_t commandReadSectors = 0x20;
static constexpr uint8_t commandWriteSectors = 0x30;
static constexpr uint8_t commandFlushCache = 0xE7;
static constexpr uint8_t commandIdentify = 0xEC;

static constexpr uint64_t maxLba28Sectors = 1ULL << 28;
static constexpr uint32_t maxSectorsPerCommand = 256; // a sector count of 0 means 256

// A drive answers within milliseconds; the limit only keeps a broken or
// missing one from hanging the kernel
static constexpr uint32_t pollLimit = 10000000;

static constexpr uint16_t wordsPerSector = BlockDevice::sectorSize / 2;

ATAPioDevice::ATAPioDevice(Channel channel, Drive drive, const char* name)
    : m_ioBase(channel == Channel::Primary ? 0x1F0 : 0x170)
    , m_controlBase(channel == Channel::Primary ? 0x3F6 : 0x376)
    , m_drive(drive)
{
    strncpy(m_name, name, sizeof(m_name) - 1);
    IOPort::outportb(m_controlBase, controlNoInterrupt);
    m_present = identify();
}

bool ATAPioDevice::waitNotBusy()
{
    for (uint32_t i = 0; i < pollLimit; i++) {
        uint8_t status = IOPort::inportb(m_ioBase + regStatus);
        if (!(status & statusBusy))
            return !(status & (statusError | statusDeviceFault));
    }
    return false;
}

bool ATAPioDevice::waitDataRequest()
{
    for (uint32_t i = 0; i < pollLimit; i++) {
        uint8_t status = IOPort::inportb(m_ioBase + regStatus);
        if (status & (statusError | statusDeviceFault))
            return false;
        if (!(status & statusBusy) && (status & statusDataRequest))
            return true;
    }
    return false;
}

void ATAPioDevice::selectDrive(uint64_t lba)
{
    uint8_t slave = m_drive == Drive::Slave ? 0x10 : 0x00;
    IOPort::outportb(m_ioBase + regDrive, 0xE0 | slave | ((lba >> 24) & 0x0F));

    // The drive needs 400 ns to put its status on the bus: four status reads
    for (int i = 0; i < 4; i++)
        IOPort::inportb(m_controlBase);
}

bool ATAPioDevice::identify()
{
    selectDrive(0);

    // Nobody drives the bus: the status reads as 0xFF
    if (IOPort::inportb(m_ioBase + regStatus) == 0xFF)
        return false;

    IOPort::outportb(m_ioBase + regSectorCount, 0);
    IOPort::outportb(m_ioBase + regLba0, 0);
    IOPort::outportb(m_ioBase + regLba1, 0);
    IOPort::outportb(m_ioBase + regLba2, 0);
    IOPort::outportb(m_ioBase + regCommand, commandIdentify);

    // Status 0: no drive
    if (!IOPort::inportb(m_ioBase + regStatus))
        return false;

    for (uint32_t i = 0; i < pollLimit; i++) {
        if (!(IOPort::inportb(m_ioBase + regStatus) & statusBusy))
            break;
    }

    // ATAPI and SATA devices put a signature in LBA1/LBA2 and abort IDENTIFY
    if (IOPort::inportb(m_ioBase + regLba1) || IOPort::inportb(m_ioBase + regLba2))
        return false;

    if (!waitDataRequest())
        return false;

    uint16_t data[wordsPerSector];
    for (uint16_t& word : data)
        word = IOPort::inportw(m_ioBase + regData);

    // Words 60..61: number of sectors addressable with 28-bit LBA (0: no LBA)
    m_sectorCount = data[60] | ((uint32_t)data[61] << 16);
    if (!m_sectorCount)
        return false;

    // Words 27..46: the model, two characters per word, high byte first
    for (size_t i = 0; i < 20; i++) {
        m_model[i * 2] = (char)(data[27 + i] >> 8);
        m_model[i * 2 + 1] = (char)(data[27 + i] & 0xFF);
    }
    for (int i = 39; i >= 0 && m_model[i] == ' '; i--)
        m_model[i] = '\0';

    return true;
}

bool ATAPioDevice::startCommand(uint64_t lba, uint32_t count, uint8_t command)
{
    if (!waitNotBusy())
        return false;

    selectDrive(lba);
    IOPort::outportb(m_ioBase + regSectorCount, count & 0xFF); // 256 -> 0
    IOPort::outportb(m_ioBase + regLba0, lba & 0xFF);
    IOPort::outportb(m_ioBase + regLba1, (lba >> 8) & 0xFF);
    IOPort::outportb(m_ioBase + regLba2, (lba >> 16) & 0xFF);
    IOPort::outportb(m_ioBase + regCommand, command);
    return true;
}

bool ATAPioDevice::readSectors(uint64_t lba, uint32_t count, void* buffer)
{
    if (!m_present || !isSectorRangeValid(lba, count, m_sectorCount) || lba + count > maxLba28Sectors)
        return false;

    auto* words = static_cast<uint16_t*>(buffer);
    while (count) {
        uint32_t chunk = count < maxSectorsPerCommand ? count : maxSectorsPerCommand;
        if (!startCommand(lba, chunk, commandReadSectors))
            return false;

        // The drive raises DRQ once per sector
        for (uint32_t sector = 0; sector < chunk; sector++) {
            if (!waitDataRequest())
                return false;
            for (uint16_t i = 0; i < wordsPerSector; i++)
                *words++ = IOPort::inportw(m_ioBase + regData);
        }

        lba += chunk;
        count -= chunk;
    }
    return true;
}

bool ATAPioDevice::writeSectors(uint64_t lba, uint32_t count, const void* buffer)
{
    if (!m_present || !isSectorRangeValid(lba, count, m_sectorCount) || lba + count > maxLba28Sectors)
        return false;

    const auto* words = static_cast<const uint16_t*>(buffer);
    while (count) {
        uint32_t chunk = count < maxSectorsPerCommand ? count : maxSectorsPerCommand;
        if (!startCommand(lba, chunk, commandWriteSectors))
            return false;

        for (uint32_t sector = 0; sector < chunk; sector++) {
            if (!waitDataRequest())
                return false;
            for (uint16_t i = 0; i < wordsPerSector; i++)
                IOPort::outportw(m_ioBase + regData, *words++);
        }

        lba += chunk;
        count -= chunk;
    }

    // The drive may keep written sectors in its cache: make them stick
    IOPort::outportb(m_ioBase + regCommand, commandFlushCache);
    return waitNotBusy();
}

} // namespace Drivers
