/**
 * @file processor_information.hpp
 * @author Oguzhan KAYNAK
 * @brief SMBIOS type 4: a processor socket and what is in it.
 */

#pragma once

#include <stdint.h>

namespace Firmware {

class SmbiosStructure;

struct ProcessorInformation {
    const char* socket; ///< the socket's name on the board
    uint8_t processorType; ///< 3: central processor
    uint8_t family; ///< 0x0B..: Pentium, 0xC6: Core i7, 0xFE: see family2 (not read)
    const char* manufacturer;
    uint64_t id; ///< x86: CPUID.1 EAX (low half) and EDX (high half)
    const char* version;
    uint16_t externalClockMhz;
    uint16_t maxSpeedMhz;
    uint16_t currentSpeedMhz;
    uint8_t status; ///< bit 6: the socket is populated; bits 2..0: 1 enabled
    const char* serialNumber; ///< 2.3 and later
    const char* assetTag;
    const char* partNumber;
    uint8_t coreCount; ///< 2.5 and later; 0 when not known
    uint8_t coresEnabled;
    uint8_t threadCount;
};

/// false if structure is not a type 4 of at least the 2.0 length
bool decodeProcessorInformation(const SmbiosStructure& structure, ProcessorInformation& info);

} // namespace Firmware
