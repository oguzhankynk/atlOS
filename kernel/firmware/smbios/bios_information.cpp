/**
 * @file bios_information.cpp
 * @author Oguzhan KAYNAK
 * @brief Decoding SMBIOS type 0.
 */

#include <firmware/smbios/bios_information.hpp>

#include <firmware/smbios/smbios.hpp>

namespace Firmware {

bool decodeBiosInformation(const SmbiosStructure& s, BiosInformation& info)
{
    if (s.type() != SmbiosBios || !s.has(0x0A, 8))
        return false;

    info.vendor = s.stringAt(0x04);
    info.version = s.stringAt(0x05);
    info.startSegment = s.word(0x06);
    info.releaseDate = s.stringAt(0x08);
    info.romSizeKb = 64 * (s.byte(0x09) + 1); // in 64 KB units, minus one
    info.characteristics = s.qword(0x0A);
    info.hasRelease = s.has(0x14, 4);
    info.releaseMajor = s.byte(0x14);
    info.releaseMinor = s.byte(0x15);
    info.controllerMajor = s.byte(0x16);
    info.controllerMinor = s.byte(0x17);
    return true;
}

} // namespace Firmware
