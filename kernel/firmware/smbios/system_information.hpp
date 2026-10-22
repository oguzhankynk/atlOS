/**
 * @file system_information.hpp
 * @author Oguzhan KAYNAK
 * @brief SMBIOS type 1: the machine (maker, product, serial number, UUID).
 */

#pragma once

#include <stdint.h>

namespace Firmware {

class SmbiosStructure;

struct SystemInformation {
    const char* manufacturer;
    const char* product;
    const char* version;
    const char* serialNumber;
    bool hasUuid; ///< 2.1 and later
    uint8_t uuid[16]; ///< as stored: since 2.6 the first three fields are little endian
    uint8_t wakeUpType; ///< 6: power switch, ...
    const char* sku; ///< 2.4 and later
    const char* family;
};

/// false if structure is not a type 1 of at least the 2.0 length
bool decodeSystemInformation(const SmbiosStructure& structure, SystemInformation& info);

} // namespace Firmware
