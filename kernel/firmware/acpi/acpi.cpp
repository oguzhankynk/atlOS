/**
 * @file acpi.cpp
 * @author Oguzhan KAYNAK
 * @brief Firmware::AcpiTables: from the RSDP to the list of tables.
 */

#include <firmware/acpi/acpi.hpp>

#include <stdio.h>
#include <string.h>

#include <arch/x86_64/boot/multiboot2.inc>
#include <mm/vmm.hpp>

namespace Firmware {

/// The ACPI 1.0 part of the RSDP, what its checksum covers
static constexpr size_t rsdpVersion1Size = 20;

bool acpiChecksumIsValid(const void* data, size_t size)
{
    auto bytes = static_cast<const uint8_t*>(data);
    uint8_t sum = 0;
    for (size_t i = 0; i < size; i++)
        sum += bytes[i];
    return sum == 0;
}

const AcpiRsdp* findRsdp(uint64_t multibootInfo)
{
    // The information starts with its size and a reserved word, then the tags (8 byte aligned)
    auto tag = reinterpret_cast<const multiboot_tag*>(multibootInfo + 8);
    const AcpiRsdp* rsdp = nullptr;
    for (; tag->type != MULTIBOOT_TAG_TYPE_END;
         tag = reinterpret_cast<const multiboot_tag*>(reinterpret_cast<const uint8_t*>(tag) + ((tag->size + 7) & ~7))) {
        if (tag->type == MULTIBOOT_TAG_TYPE_ACPI_NEW)
            return reinterpret_cast<const AcpiRsdp*>(reinterpret_cast<const multiboot_tag_new_acpi*>(tag)->rsdp);
        if (tag->type == MULTIBOOT_TAG_TYPE_ACPI_OLD)
            rsdp = reinterpret_cast<const AcpiRsdp*>(reinterpret_cast<const multiboot_tag_old_acpi*>(tag)->rsdp);
    }
    return rsdp;
}

/// A table header at physical, or nullptr if the table is not inside the physmap
static const AcpiTableHeader* mapTable(uint64_t physical)
{
    if (!physical || physical > Memory::physmapSize - sizeof(AcpiTableHeader))
        return nullptr;
    auto header = static_cast<const AcpiTableHeader*>(Memory::physToVirt(physical));
    if (header->length < sizeof(AcpiTableHeader) || header->length > Memory::physmapSize - physical)
        return nullptr;
    return header;
}

bool AcpiTables::load(const AcpiRsdp& rsdp)
{
    m_root = nullptr;
    m_count = 0;

    if (memcmp(rsdp.signature, "RSD PTR ", 8) != 0 || !acpiChecksumIsValid(&rsdp, rsdpVersion1Size)) {
        printf("ACPI: the RSDP is not valid\n");
        return false;
    }

    // ACPI 2.0 and later: the XSDT, with 64 bit addresses. Its checksum covers the whole RSDP.
    bool extended = rsdp.revision >= 2 && rsdp.xsdtAddress && acpiChecksumIsValid(&rsdp, rsdp.length);
    uint64_t rootPhysical = extended ? rsdp.xsdtAddress : rsdp.rsdtAddress;
    const AcpiTableHeader* root = mapTable(rootPhysical);
    if (!root || memcmp(root->signature, extended ? "XSDT" : "RSDT", 4) != 0
        || !acpiChecksumIsValid(root, root->length)) {
        printf("ACPI: the %s at 0x%lx is not valid\n", extended ? "XSDT" : "RSDT", rootPhysical);
        return false;
    }

    m_root = root;
    m_rootPhysical = rootPhysical;
    m_revision = rsdp.revision;
    memcpy(m_oemId, rsdp.oemId, sizeof(rsdp.oemId));

    // After the header: the table addresses, 8 or 4 bytes each (not always aligned)
    size_t entrySize = extended ? 8 : 4;
    auto entries = reinterpret_cast<const uint8_t*>(root) + sizeof(AcpiTableHeader);
    size_t entryCount = (root->length - sizeof(AcpiTableHeader)) / entrySize;
    for (size_t i = 0; i < entryCount; i++) {
        uint64_t physical = 0;
        memcpy(&physical, entries + i * entrySize, entrySize);
        add(physical);
    }
    return true;
}

void AcpiTables::add(uint64_t physical)
{
    const AcpiTableHeader* header = mapTable(physical);
    if (!header) {
        printf("ACPI: skipping a table at 0x%lx, outside the physmap\n", physical);
        return;
    }
    if (m_count == maxTables) {
        printf("ACPI: more than %u tables, skipping %.4s\n", maxTables, header->signature);
        return;
    }
    m_tables[m_count++] = { physical, header, acpiChecksumIsValid(header, header->length) };
}

const AcpiTableHeader* AcpiTables::find(const char* signature) const
{
    for (uint32_t i = 0; i < m_count; i++) {
        if (m_tables[i].checksumValid && !memcmp(m_tables[i].header->signature, signature, 4))
            return m_tables[i].header;
    }
    return nullptr;
}

} // namespace Firmware
