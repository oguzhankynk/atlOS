/**
 * @file ata.hpp
 * @author Oguzhan KAYNAK
 * @brief Defines the ATA class that implements disk operations using the ATA protocol.
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

#ifndef D2A30C7E_444F_4393_BCBF_81177EB71649
#define D2A30C7E_444F_4393_BCBF_81177EB71649

#include <stdint.h>

#include <drivers/disk/disk.hpp>

/**
 *
 * @class ATA
 * @brief Represents an ATA disk driver for reading and writing sectors.
 *
 */
class ATA : public Disk
{
public:
    ATA(const char *name) noexcept;
    virtual ~ATA() noexcept = default;

    /**
     * @brief Reads a single sector from the disk into the buffer.
     *
     * @param lba The Logical Block Address (LBA) of the sector to read.
     * @param buffer Pointer to the buffer where the data will be stored.
     */
    void read_sector(uint32_t lba, uint8_t *buffer) const override;

    /**
     * @brief Reads multiple sectors from the disk into the buffer.
     *
     * @param lba_start The starting LBA of the first sector to read.
     * @param buffer Pointer to the buffer where the data will be stored.
     * @param size The size of data to read in bytes.
     */
    void read(uint32_t lba_start, uint8_t *buffer, size_t size) const override;

    /**
     * @brief Writes a single sector to the disk from the buffer.
     *
     * @param lba The Logical Block Address (LBA) of the sector to write.
     * @param buffer Pointer to the buffer containing the data to write.
     */
    void write_sector(uint32_t lba, const uint8_t *buffer) const override;

    /**
     * @brief Writes multiple sectors to the disk from the buffer.
     *
     * @param lba_start The starting LBA of the first sector to write.
     * @param buffer Pointer to the buffer containing the data to write.
     * @param size The size of data to write in bytes.
     */
    void write(uint32_t lba_start, const uint8_t *buffer, size_t size) const override;
};
#endif /* D2A30C7E_444F_4393_BCBF_81177EB71649 */
