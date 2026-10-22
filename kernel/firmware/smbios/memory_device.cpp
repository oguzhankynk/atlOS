/**
 * @file memory_device.cpp
 * @author Oguzhan KAYNAK
 * @brief Decoding SMBIOS type 17.
 */

#include <firmware/smbios/memory_device.hpp>

#include <firmware/smbios/smbios.hpp>

namespace Firmware {

// The size word: 0 no module, 0xFFFF unknown, 0x7FFF "see the extended size"
// (2.7, a dword in MB); bit 15 set: the rest is in KB, clear: in MB
static constexpr uint16_t sizeUnknown = 0xFFFF;
static constexpr uint16_t sizeExtended = 0x7FFF;
static constexpr uint16_t sizeInKb = 0x8000;

bool decodeMemoryDevice(const SmbiosStructure& s, MemoryDevice& info)
{
    if (s.type() != SmbiosMemoryDevice || !s.has(0x04, 0x11))
        return false;

    info.arrayHandle = s.word(0x04);
    info.totalWidth = s.word(0x08);
    info.dataWidth = s.word(0x0A);

    uint16_t size = s.word(0x0C);
    info.sizeKnown = size != sizeUnknown;
    if (!info.sizeKnown)
        info.sizeKb = 0;
    else if (size == sizeExtended && s.has(0x1C, 4))
        info.sizeKb = (uint64_t)(s.dword(0x1C) & 0x7FFFFFFF) * 1024;
    else if (size & sizeInKb)
        info.sizeKb = size & ~sizeInKb;
    else
        info.sizeKb = (uint64_t)size * 1024;

    info.formFactor = s.byte(0x0E);
    info.deviceLocator = s.stringAt(0x10);
    info.bankLocator = s.stringAt(0x11);
    info.memoryType = s.byte(0x12);
    info.speedMts = s.word(0x15);
    info.manufacturer = s.stringAt(0x17);
    info.serialNumber = s.stringAt(0x18);
    info.assetTag = s.stringAt(0x19);
    info.partNumber = s.stringAt(0x1A);
    info.configuredSpeedMts = s.word(0x20);
    return true;
}

const char* smbiosMemoryTypeName(uint8_t memoryType)
{
    switch (memoryType) {
    case 0x01:
        return "Other";
    case 0x02:
        return "Unknown";
    case 0x03:
        return "DRAM";
    case 0x07:
        return "RAM";
    case 0x0F:
        return "SDRAM";
    case 0x12:
        return "DDR";
    case 0x13:
        return "DDR2";
    case 0x18:
        return "DDR3";
    case 0x1A:
        return "DDR4";
    case 0x1B:
        return "LPDDR";
    case 0x1C:
        return "LPDDR2";
    case 0x1D:
        return "LPDDR3";
    case 0x1E:
        return "LPDDR4";
    case 0x22:
        return "DDR5";
    case 0x23:
        return "LPDDR5";
    default:
        return "";
    }
}

} // namespace Firmware
