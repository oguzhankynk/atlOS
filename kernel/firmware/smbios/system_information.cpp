/**
 * @file system_information.cpp
 * @author Oguzhan KAYNAK
 * @brief Decoding SMBIOS type 1.
 */

#include <firmware/smbios/system_information.hpp>

#include <string.h>

#include <firmware/smbios/smbios.hpp>

namespace Firmware {

bool decodeSystemInformation(const SmbiosStructure& s, SystemInformation& info)
{
    if (s.type() != SmbiosSystem || !s.has(0x04, 4))
        return false;

    info.manufacturer = s.stringAt(0x04);
    info.product = s.stringAt(0x05);
    info.version = s.stringAt(0x06);
    info.serialNumber = s.stringAt(0x07);
    info.hasUuid = s.has(0x08, 16);
    memset(info.uuid, 0, sizeof(info.uuid));
    if (info.hasUuid)
        memcpy(info.uuid, s.data() + 0x08, sizeof(info.uuid));
    info.wakeUpType = s.byte(0x18);
    info.sku = s.stringAt(0x19);
    info.family = s.stringAt(0x1A);
    return true;
}

} // namespace Firmware
