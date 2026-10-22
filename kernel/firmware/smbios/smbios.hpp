/**
 * @file smbios.hpp
 * @author Oguzhan KAYNAK
 * @brief SMBIOS 2.x: finding the entry point in the BIOS area, walking the
 * structure table, reading a structure's fields and strings.
 */

#pragma once

#include <stddef.h>
#include <stdint.h>

namespace Firmware {

/// Where a BIOS puts the 2.x entry point: on a 16 byte boundary in 0xF0000 .. 0xFFFFF
static constexpr uint64_t smbiosAreaStart = 0xF0000;
static constexpr uint64_t smbiosAreaSize = 0x10000;

/// The SMBIOS 2.1 entry point (31 bytes)
struct [[gnu::packed]] SmbiosEntryPoint {
    char anchor[4]; ///< "_SM_"
    uint8_t checksum; ///< the length bytes add up to 0
    uint8_t length; ///< 0x1F
    uint8_t majorVersion;
    uint8_t minorVersion;
    uint16_t maxStructureSize;
    uint8_t revision;
    uint8_t formattedArea[5];
    char intermediateAnchor[5]; ///< "_DMI_"
    uint8_t intermediateChecksum; ///< the 15 bytes from intermediateAnchor add up to 0
    uint16_t tableLength; ///< of the structure table, in bytes
    uint32_t tableAddress; ///< physical address of the structure table
    uint16_t structureCount;
    uint8_t bcdRevision;
};

/// The anchors, both checksums and a sane length
bool smbiosEntryIsValid(const SmbiosEntryPoint& entry);

/**
 * @brief Look for a valid 2.x entry point ("_SM_") on the 16 byte boundaries
 * of [area, area + size). A 3.0 entry point ("_SM3_") is not read; seenVersion3
 * tells whether there was one.
 * @return The entry point, or nullptr.
 */
const SmbiosEntryPoint* findSmbios(const void* area, size_t size, bool& seenVersion3);

/**
 * @brief One structure of the table: the formatted area (type, length, handle,
 * then the fields of its type) followed by its strings, each ended by a NUL,
 * the set ended by one more NUL. Fields refer to strings by number, from 1.
 */
class SmbiosStructure {
public:
    SmbiosStructure() = default;
    SmbiosStructure(const uint8_t* data, const char* strings, uint32_t stringsSize)
        : m_data(data)
        , m_strings(strings)
        , m_stringsSize(stringsSize)
    {
    }

    uint8_t type() const { return m_data[0]; }
    uint8_t length() const { return m_data[1]; } ///< of the formatted area
    uint16_t handle() const { return word(2); }
    const uint8_t* data() const { return m_data; }
    uint32_t stringsSize() const { return m_stringsSize; }

    /// The formatted area has [offset, offset + size): older versions have shorter structures
    bool has(uint32_t offset, uint32_t size) const { return offset + size <= length(); }

    /// Little endian fields; 0 beyond the formatted area
    uint8_t byte(uint32_t offset) const { return has(offset, 1) ? m_data[offset] : 0; }
    uint16_t word(uint32_t offset) const;
    uint32_t dword(uint32_t offset) const;
    uint64_t qword(uint32_t offset) const;

    /// String number (1 ..); "" for 0 (none) and for a number past the last string
    const char* string(uint8_t number) const;

    /// The string whose number is the byte at offset
    const char* stringAt(uint32_t offset) const { return string(byte(offset)); }

    uint32_t stringCount() const;

private:
    const uint8_t* m_data {};
    const char* m_strings {};
    uint32_t m_stringsSize {};
};

/// The kinds of structure atlOS decodes (one file each); smbiosTypeName knows more
enum SmbiosType : uint8_t {
    SmbiosBios = 0,
    SmbiosSystem = 1,
    SmbiosBaseboard = 2,
    SmbiosProcessor = 4,
    SmbiosMemoryDevice = 17,
    SmbiosEndOfTable = 127,
};

/// The name of a structure type ("Processor"), "" for one atlOS does not know
const char* smbiosTypeName(uint8_t type);

/**
 * @brief The structures of the table the entry point describes, read in
 * place through the physmap.
 */
class SmbiosTables {
public:
    static constexpr uint32_t maxStructures = 128;

    SmbiosTables() = default;
    SmbiosTables(const SmbiosTables&) = delete;
    SmbiosTables& operator=(const SmbiosTables&) = delete;

    /**
     * @brief Check the entry point and list the structures, up to the end of
     * table structure (type 127) or the table's length. A structure that runs
     * past the table ends the list there (truncated()).
     * @return false (nothing listed) if the entry point is not valid or the table is outside the physmap.
     */
    bool load(const SmbiosEntryPoint& entry);

    bool isLoaded() const { return m_loaded; }
    uint8_t majorVersion() const { return m_major; }
    uint8_t minorVersion() const { return m_minor; }
    uint64_t tableAddress() const { return m_tableAddress; }
    uint32_t tableLength() const { return m_tableLength; }

    /// A structure ran past the end of the table; the ones before it are listed
    bool truncated() const { return m_truncated; }

    uint32_t count() const { return m_count; }
    const SmbiosStructure& at(uint32_t index) const { return m_structures[index]; }

    /// The index-th structure (from 0) of this type, or nullptr
    const SmbiosStructure* find(uint8_t type, uint32_t index = 0) const;

private:
    bool m_loaded {};
    bool m_truncated {};
    uint8_t m_major {};
    uint8_t m_minor {};
    uint64_t m_tableAddress {};
    uint32_t m_tableLength {};
    SmbiosStructure m_structures[maxStructures] {};
    uint32_t m_count {};
};

} // namespace Firmware
