/**
 * @file processor_information.cpp
 * @author Oguzhan KAYNAK
 * @brief Decoding SMBIOS type 4.
 */

#include <firmware/smbios/processor_information.hpp>

#include <firmware/smbios/smbios.hpp>

namespace Firmware {

bool decodeProcessorInformation(const SmbiosStructure& s, ProcessorInformation& info)
{
    if (s.type() != SmbiosProcessor || !s.has(0x04, 0x16))
        return false;

    info.socket = s.stringAt(0x04);
    info.processorType = s.byte(0x05);
    info.family = s.byte(0x06);
    info.manufacturer = s.stringAt(0x07);
    info.id = s.qword(0x08);
    info.version = s.stringAt(0x10);
    info.externalClockMhz = s.word(0x12);
    info.maxSpeedMhz = s.word(0x14);
    info.currentSpeedMhz = s.word(0x16);
    info.status = s.byte(0x18);
    info.serialNumber = s.stringAt(0x20);
    info.assetTag = s.stringAt(0x21);
    info.partNumber = s.stringAt(0x22);
    info.coreCount = s.byte(0x23);
    info.coresEnabled = s.byte(0x24);
    info.threadCount = s.byte(0x25);
    return true;
}

} // namespace Firmware
