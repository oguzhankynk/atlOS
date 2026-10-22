/**
 * @file bios_information.hpp
 * @author Oguzhan KAYNAK
 * @brief SMBIOS type 0: the BIOS (firmware) itself.
 */

#pragma once

#include <stdint.h>

namespace Firmware {

class SmbiosStructure;

struct BiosInformation {
    const char* vendor;
    const char* version;
    const char* releaseDate; ///< mm/dd/yyyy
    uint16_t startSegment; ///< where the BIOS starts in real mode (0xE800: 0xE8000)
    uint32_t romSizeKb;
    uint64_t characteristics; ///< bit 3: the characteristics are not supported, 7: PCI, ...
    bool hasRelease; ///< 2.4 and later: the release numbers below
    uint8_t releaseMajor;
    uint8_t releaseMinor;
    uint8_t controllerMajor; ///< embedded controller firmware; 0xFF: none
    uint8_t controllerMinor;
};

/// false if structure is not a type 0 of at least the 2.0 length
bool decodeBiosInformation(const SmbiosStructure& structure, BiosInformation& info);

} // namespace Firmware
