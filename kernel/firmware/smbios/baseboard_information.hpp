/**
 * @file baseboard_information.hpp
 * @author Oguzhan KAYNAK
 * @brief SMBIOS type 2: the motherboard.
 */

#pragma once

#include <stdint.h>

namespace Firmware {

class SmbiosStructure;

struct BaseboardInformation {
    const char* manufacturer;
    const char* product;
    const char* version;
    const char* serialNumber;
    const char* assetTag;
    uint8_t features; ///< bit 0: a hosting board (the motherboard), 3: replaceable, ...
    const char* locationInChassis;
    uint16_t chassisHandle;
    uint8_t boardType; ///< 0x0A: motherboard; 0 if the structure is too short
};

/// false if structure is not a type 2 with at least the strings up to the serial number
bool decodeBaseboardInformation(const SmbiosStructure& structure, BaseboardInformation& info);

} // namespace Firmware
