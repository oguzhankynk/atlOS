/**
 * @file smbios.cpp
 * @author Oguzhan KAYNAK
 * @brief Finding the SMBIOS entry point and walking the structure table.
 */

#include <firmware/smbios/smbios.hpp>

#include <stdio.h>
#include <string.h>

#include <mm/vmm.hpp>

namespace Firmware {

/// The checksum of "_DMI_" covers 15 bytes: the intermediate anchor to the end
static constexpr size_t intermediateSize = 15;
static constexpr size_t intermediateOffset = 0x10;

/// The formatted area starts with type, length and handle
static constexpr uint32_t structureHeaderSize = 4;

static uint8_t byteSum(const void* data, size_t size)
{
    auto bytes = static_cast<const uint8_t*>(data);
    uint8_t sum = 0;
    for (size_t i = 0; i < size; i++)
        sum += bytes[i];
    return sum;
}

bool smbiosEntryIsValid(const SmbiosEntryPoint& entry)
{
    auto bytes = reinterpret_cast<const uint8_t*>(&entry);
    // 0x1F, or 0x1E from firmware that followed an error in the 2.1 specification
    return memcmp(entry.anchor, "_SM_", 4) == 0 && entry.length >= 0x1E && entry.length <= 0x20 && byteSum(&entry, entry.length) == 0
        && memcmp(entry.intermediateAnchor, "_DMI_", 5) == 0 && byteSum(bytes + intermediateOffset, intermediateSize) == 0;
}

const SmbiosEntryPoint* findSmbios(const void* area, size_t size, bool& seenVersion3)
{
    seenVersion3 = false;
    auto bytes = static_cast<const uint8_t*>(area);
    for (size_t offset = 0; offset + sizeof(SmbiosEntryPoint) <= size; offset += 16) {
        if (memcmp(bytes + offset, "_SM3_", 5) == 0)
            seenVersion3 = true;
        auto entry = reinterpret_cast<const SmbiosEntryPoint*>(bytes + offset);
        if (smbiosEntryIsValid(*entry))
            return entry;
    }
    return nullptr;
}

uint16_t SmbiosStructure::word(uint32_t offset) const
{
    return byte(offset) | (byte(offset + 1) << 8);
}

uint32_t SmbiosStructure::dword(uint32_t offset) const
{
    return word(offset) | ((uint32_t)word(offset + 2) << 16);
}

uint64_t SmbiosStructure::qword(uint32_t offset) const
{
    return dword(offset) | ((uint64_t)dword(offset + 4) << 32);
}

const char* SmbiosStructure::string(uint8_t number) const
{
    // The set ends with an empty string: the strlen of each one stays inside it
    const char* s = m_strings;
    for (uint8_t n = 1; number && *s; n++) {
        if (n == number)
            return s;
        s += strlen(s) + 1;
    }
    return "";
}

uint32_t SmbiosStructure::stringCount() const
{
    uint32_t count = 0;
    for (const char* s = m_strings; *s; s += strlen(s) + 1)
        count++;
    return count;
}

const char* smbiosTypeName(uint8_t type)
{
    switch (type) {
    case 0:
        return "BIOS";
    case 1:
        return "System";
    case 2:
        return "Baseboard";
    case 3:
        return "Chassis";
    case 4:
        return "Processor";
    case 7:
        return "Cache";
    case 8:
        return "Port connector";
    case 9:
        return "System slot";
    case 11:
        return "OEM strings";
    case 16:
        return "Physical memory array";
    case 17:
        return "Memory device";
    case 19:
        return "Memory array mapped address";
    case 32:
        return "System boot";
    case 127:
        return "End of table";
    default:
        return type >= 128 ? "OEM specific" : "";
    }
}

bool SmbiosTables::load(const SmbiosEntryPoint& entry)
{
    m_loaded = false;
    m_truncated = false;
    m_count = 0;

    if (!smbiosEntryIsValid(entry)) {
        printf("SMBIOS: the entry point is not valid\n");
        return false;
    }
    if (entry.tableAddress + (uint64_t)entry.tableLength > Memory::physmapSize) {
        printf("SMBIOS: the table at 0x%x is outside the physmap\n", entry.tableAddress);
        return false;
    }

    m_loaded = true;
    m_major = entry.majorVersion;
    m_minor = entry.minorVersion;
    m_tableAddress = entry.tableAddress;
    m_tableLength = entry.tableLength;

    auto table = static_cast<const uint8_t*>(Memory::physToVirt(entry.tableAddress));
    uint32_t offset = 0;
    while (offset + structureHeaderSize <= m_tableLength && m_count < maxStructures) {
        uint8_t length = table[offset + 1];
        if (length < structureHeaderSize || length > m_tableLength - offset) {
            m_truncated = true;
            break;
        }

        // The strings: up to the two NULs in a row (an empty set is just those two)
        uint32_t stringsStart = offset + length;
        uint32_t end = stringsStart;
        while (end + 1 < m_tableLength && (table[end] || table[end + 1]))
            end++;
        if (end + 1 >= m_tableLength) {
            m_truncated = true;
            break;
        }

        uint8_t type = table[offset];
        m_structures[m_count++] = SmbiosStructure(table + offset, reinterpret_cast<const char*>(table + stringsStart),
            end + 2 - stringsStart);
        offset = end + 2;
        if (type == SmbiosEndOfTable)
            break;
    }
    if (m_truncated)
        printf("SMBIOS: a structure at offset 0x%x runs past the table\n", offset);
    return true;
}

const SmbiosStructure* SmbiosTables::find(uint8_t type, uint32_t index) const
{
    for (uint32_t i = 0; i < m_count; i++) {
        if (m_structures[i].type() == type && index-- == 0)
            return &m_structures[i];
    }
    return nullptr;
}

} // namespace Firmware
