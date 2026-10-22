/**
 * @file memory_device.hpp
 * @author Oguzhan KAYNAK
 * @brief SMBIOS type 17: a memory slot and the module in it.
 */

#pragma once

#include <stdint.h>

namespace Firmware {

class SmbiosStructure;

struct MemoryDevice {
    uint16_t arrayHandle; ///< the type 16 structure (the memory array) it belongs to
    uint16_t totalWidth; ///< bits, with ECC; 0xFFFF: unknown
    uint16_t dataWidth;
    bool sizeKnown;
    uint64_t sizeKb; ///< 0: the slot is empty
    uint8_t formFactor; ///< 0x09: DIMM, 0x0D: SODIMM, ...
    const char* deviceLocator; ///< the slot's name ("DIMM 0")
    const char* bankLocator;
    uint8_t memoryType; ///< 0x18: DDR3, 0x1A: DDR4, 0x22: DDR5, ... (smbiosMemoryTypeName)
    uint16_t speedMts; ///< 2.3 and later; megatransfers per second, 0: unknown
    const char* manufacturer;
    const char* serialNumber;
    const char* assetTag;
    const char* partNumber;
    uint16_t configuredSpeedMts; ///< 2.7 and later
};

/// false if structure is not a type 17 of at least the 2.1 length
bool decodeMemoryDevice(const SmbiosStructure& structure, MemoryDevice& info);

/// "DDR4", "DRAM", ...; "" for a type atlOS does not know
const char* smbiosMemoryTypeName(uint8_t memoryType);

} // namespace Firmware
