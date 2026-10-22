/**
 * @file acpi.hpp
 * @author Oguzhan KAYNAK
 * @brief The ACPI tables: the RSDP that multiboot hands over, the root table
 * (RSDT or XSDT) and the tables it points at, with their checksums checked.
 */

#pragma once

#include <stddef.h>
#include <stdint.h>

namespace Firmware {

/// Root System Description Pointer: where the firmware's tables start
struct [[gnu::packed]] AcpiRsdp {
    char signature[8]; ///< "RSD PTR "
    uint8_t checksum; ///< the first 20 bytes add up to 0
    char oemId[6];
    uint8_t revision; ///< 0: ACPI 1.0, only the RSDT; 2 and later: the XSDT too
    uint32_t rsdtAddress; ///< physical address of the RSDT (32 bit entries)

    // Revision 2 and later
    uint32_t length; ///< of this structure
    uint64_t xsdtAddress; ///< physical address of the XSDT (64 bit entries)
    uint8_t extendedChecksum; ///< all length bytes add up to 0
    uint8_t reserved[3];
};

/// The header every ACPI table starts with
struct [[gnu::packed]] AcpiTableHeader {
    char signature[4]; ///< "APIC" (the MADT), "FACP" (the FADT), "HPET", ...
    uint32_t length; ///< the whole table, header included
    uint8_t revision;
    uint8_t checksum; ///< all length bytes add up to 0
    char oemId[6];
    char oemTableId[8];
    uint32_t oemRevision;
    uint32_t creatorId;
    uint32_t creatorRevision;
};

/// The bytes add up to 0 (modulo 256): how every ACPI structure is checked
bool acpiChecksumIsValid(const void* data, size_t size);

/**
 * @brief The RSDP in the multiboot information (GRUB copies it there), the
 * ACPI 2.0 one if there are both. nullptr if the firmware has no ACPI.
 */
const AcpiRsdp* findRsdp(uint64_t multibootInfo);

/**
 * @brief The tables the root table (the XSDT, or the RSDT on ACPI 1.0) lists.
 * They are read in place through the physmap; nothing is copied.
 */
class AcpiTables {
public:
    static constexpr uint32_t maxTables = 32;

    struct Table {
        uint64_t physical;
        const AcpiTableHeader* header;
        bool checksumValid; ///< a table with a wrong checksum is listed, but find skips it
    };

    AcpiTables() = default;
    AcpiTables(const AcpiTables&) = delete;
    AcpiTables& operator=(const AcpiTables&) = delete;

    /**
     * @brief Check the RSDP and the root table, then list the tables.
     * @return false (and nothing listed) if the RSDP or the root table is not valid.
     */
    bool load(const AcpiRsdp& rsdp);

    bool isLoaded() const { return m_root != nullptr; }

    /// The RSDP's revision: 0 for ACPI 1.0, 2 and later for an XSDT
    uint8_t revision() const { return m_revision; }
    const char* oemId() const { return m_oemId; }

    /// The root table: the XSDT, or the RSDT
    const AcpiTableHeader* root() const { return m_root; }
    uint64_t rootPhysical() const { return m_rootPhysical; }

    uint32_t count() const { return m_count; }
    const Table& at(uint32_t index) const { return m_tables[index]; }

    /// The first table with this signature and a valid checksum, or nullptr
    const AcpiTableHeader* find(const char* signature) const;

private:
    void add(uint64_t physical);

    const AcpiTableHeader* m_root {};
    uint64_t m_rootPhysical {};
    uint8_t m_revision {};
    char m_oemId[7] {};
    Table m_tables[maxTables] {};
    uint32_t m_count {};
};

} // namespace Firmware
