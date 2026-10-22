/**
 * @file madt.cpp
 * @author Oguzhan KAYNAK
 * @brief Reading the entries of the MADT.
 */

#include <firmware/acpi/madt.hpp>

#include <string.h>

#include <firmware/acpi/acpi.hpp>

namespace Firmware {

/// After the table header: the local APIC address and the flags, then the entries
static constexpr uint32_t madtEntriesOffset = sizeof(AcpiTableHeader) + 8;
static constexpr uint32_t madtFlagPcatCompatible = 1 << 0;
static constexpr uint32_t madtCpuEnabled = 1 << 0;

/// Little endian fields at any alignment
static uint16_t read16(const uint8_t* p) { return p[0] | (p[1] << 8); }
static uint32_t read32(const uint8_t* p) { return read16(p) | ((uint32_t)read16(p + 2) << 16); }
static uint64_t read64(const uint8_t* p) { return read32(p) | ((uint64_t)read32(p + 4) << 32); }

bool parseMadt(const AcpiTableHeader& table, MadtInfo& info)
{
    memset(&info, 0, sizeof(info));
    if (memcmp(table.signature, "APIC", 4) != 0 || table.length < madtEntriesOffset)
        return false;

    auto bytes = reinterpret_cast<const uint8_t*>(&table);
    info.localApicAddress = read32(bytes + sizeof(AcpiTableHeader));
    info.hasLegacyPics = read32(bytes + sizeof(AcpiTableHeader) + 4) & madtFlagPcatCompatible;

    // Each entry: its type, its length (both included), then the fields
    for (uint32_t offset = madtEntriesOffset; offset < table.length;) {
        if (table.length - offset < 2)
            return false;
        const uint8_t* entry = bytes + offset;
        uint8_t type = entry[0];
        uint8_t length = entry[1];
        if (length < 2 || length > table.length - offset)
            return false;

        if (type == MadtLocalApic && length >= 8 && info.cpuCount < MadtInfo::maxCpus)
            info.cpus[info.cpuCount++] = { entry[2], entry[3], (read32(entry + 4) & madtCpuEnabled) != 0 };
        else if (type == MadtIoApic && length >= 12 && info.ioApicCount < MadtInfo::maxIoApics)
            info.ioApics[info.ioApicCount++] = { entry[2], read32(entry + 4), read32(entry + 8) };
        else if (type == MadtSourceOverride && length >= 10 && info.overrideCount < MadtInfo::maxOverrides)
            info.overrides[info.overrideCount++] = { entry[3], read32(entry + 4), read16(entry + 8) };
        else if (type == MadtLocalApicNmi && length >= 6 && info.nmiCount < MadtInfo::maxNmis)
            info.nmis[info.nmiCount++] = { entry[2], entry[5], read16(entry + 3) };
        else if (type == MadtLocalApicAddress && length >= 12)
            info.localApicAddress = read64(entry + 4);
        else
            info.skippedCount++;

        offset += length;
    }
    return true;
}

IsaRoute isaRoute(const MadtInfo& info, uint8_t irq)
{
    for (uint32_t i = 0; i < info.overrideCount; i++) {
        const MadtInfo::Override& o = info.overrides[i];
        if (o.irq != irq)
            continue;
        // "As the bus does" is active high and edge for ISA
        return { o.gsi, (o.flags & MadtPolarityMask) == MadtPolarityLow, (o.flags & MadtTriggerMask) == MadtTriggerLevel };
    }
    return { irq, false, false };
}

} // namespace Firmware
