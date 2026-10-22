/**
 * @file baseboard_information.cpp
 * @author Oguzhan KAYNAK
 * @brief Decoding SMBIOS type 2.
 */

#include <firmware/smbios/baseboard_information.hpp>

#include <firmware/smbios/smbios.hpp>

namespace Firmware {

bool decodeBaseboardInformation(const SmbiosStructure& s, BaseboardInformation& info)
{
    if (s.type() != SmbiosBaseboard || !s.has(0x04, 4))
        return false;

    info.manufacturer = s.stringAt(0x04);
    info.product = s.stringAt(0x05);
    info.version = s.stringAt(0x06);
    info.serialNumber = s.stringAt(0x07);
    info.assetTag = s.stringAt(0x08);
    info.features = s.byte(0x09);
    info.locationInChassis = s.stringAt(0x0A);
    info.chassisHandle = s.word(0x0B);
    info.boardType = s.byte(0x0D);
    return true;
}

} // namespace Firmware
