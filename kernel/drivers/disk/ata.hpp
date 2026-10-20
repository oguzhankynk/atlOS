/**
 * @file ata.hpp
 * @author Oguzhan KAYNAK
 * @brief Drivers::ATAPioDevice: an ATA (IDE) disk read and written with
 * programmed I/O, polling the status register.
 */

#pragma once

#include <stdint.h>

#include <drivers/disk/block_device.hpp>

namespace Drivers {

class ATAPioDevice final : public BlockDevice {
public:
    enum class Channel : uint8_t {
        Primary, ///< I/O ports 0x1F0..0x1F7, control 0x3F6
        Secondary, ///< I/O ports 0x170..0x177, control 0x376
    };

    enum class Drive : uint8_t {
        Master,
        Slave,
    };

    /**
     * @brief Look for an ATA disk at channel/drive with IDENTIFY DEVICE.
     * Without one (nothing there, or an ATAPI CD drive) isPresent() is false
     * and every read fails. Turns the channel's interrupt off: the driver polls.
     */
    ATAPioDevice(Channel channel, Drive drive, const char* name);

    bool isPresent() const { return m_present; }

    /// Model string from IDENTIFY, e.g. "QEMU HARDDISK".
    const char* model() const { return m_model; }

    bool readSectors(uint64_t lba, uint32_t count, void* buffer) override;
    bool writeSectors(uint64_t lba, uint32_t count, const void* buffer) override;
    uint64_t sectorCount() const override { return m_sectorCount; }
    const char* name() const override { return m_name; }

private:
    bool identify();
    void selectDrive(uint64_t lba);
    bool startCommand(uint64_t lba, uint32_t count, uint8_t command);
    bool waitNotBusy();
    bool waitDataRequest();

    uint16_t m_ioBase;
    uint16_t m_controlBase;
    Drive m_drive;
    bool m_present {};
    uint64_t m_sectorCount {};
    char m_model[41] {};
    char m_name[16] {};
};

} // namespace Drivers
