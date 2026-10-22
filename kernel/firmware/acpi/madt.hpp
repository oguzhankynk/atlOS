/**
 * @file madt.hpp
 * @author Oguzhan KAYNAK
 * @brief The MADT ("APIC" table): the processors' local APICs, the I/O APICs,
 * and how the ISA IRQs reach them (the interrupt source overrides).
 */

#pragma once

#include <stdint.h>

namespace Firmware {

struct AcpiTableHeader;

/// The kinds of MADT entries atlOS reads; the others are counted and skipped
enum MadtEntryType : uint8_t {
    MadtLocalApic = 0, ///< a processor and its local APIC
    MadtIoApic = 1,
    MadtSourceOverride = 2, ///< an ISA IRQ that is not on the GSI of the same number
    MadtNmiSource = 3, ///< a GSI that is an NMI
    MadtLocalApicNmi = 4, ///< which LINT pin of a local APIC is the NMI
    MadtLocalApicAddress = 5, ///< a 64 bit address for the local APICs
};

/// Polarity and trigger mode flags of an override or an NMI entry
enum MadtIntiFlags : uint16_t {
    MadtPolarityMask = 0x3, ///< 0: as the bus does (ISA: active high), 1: high, 3: low
    MadtPolarityHigh = 0x1,
    MadtPolarityLow = 0x3,
    MadtTriggerMask = 0xC, ///< 0: as the bus does (ISA: edge), 4: edge, 0xC: level
    MadtTriggerEdge = 0x4,
    MadtTriggerLevel = 0xC,
};

/// What the MADT says about the machine
struct MadtInfo {
    static constexpr uint32_t maxCpus = 16;
    static constexpr uint32_t maxIoApics = 8;
    static constexpr uint32_t maxOverrides = 16;
    static constexpr uint32_t maxNmis = 8;

    struct Cpu {
        uint8_t processorId; ///< the ACPI processor id
        uint8_t apicId; ///< its local APIC's id
        bool enabled; ///< false: present, but must not be started
    };

    struct IoApic {
        uint8_t id;
        uint32_t address; ///< physical address of its registers
        uint32_t gsiBase; ///< the first global system interrupt (GSI) it handles
    };

    struct Override {
        uint8_t irq; ///< the ISA IRQ ...
        uint32_t gsi; ///< ... arrives on this GSI
        uint16_t flags; ///< MadtIntiFlags
    };

    struct Nmi {
        uint8_t processorId; ///< 0xFF: every processor
        uint8_t lint; ///< LINT0 or LINT1
        uint16_t flags;
    };

    uint64_t localApicAddress; ///< physical address of the local APIC registers
    bool hasLegacyPics; ///< the 8259 PICs are there too (and must be masked)

    Cpu cpus[maxCpus];
    uint32_t cpuCount;
    IoApic ioApics[maxIoApics];
    uint32_t ioApicCount;
    Override overrides[maxOverrides];
    uint32_t overrideCount;
    Nmi nmis[maxNmis];
    uint32_t nmiCount;
    uint32_t skippedCount; ///< entries of other kinds, or beyond the limits above
};

/**
 * @brief Read the entries of a MADT. info is cleared first.
 * @return false if table is not a MADT or an entry runs past its end.
 */
bool parseMadt(const AcpiTableHeader& table, MadtInfo& info);

/// How an ISA IRQ reaches an I/O APIC
struct IsaRoute {
    uint32_t gsi;
    bool activeLow;
    bool levelTriggered;
};

/// The route of irq (0 .. 15): its override, or the GSI of the same number, active high, edge
IsaRoute isaRoute(const MadtInfo& info, uint8_t irq);

} // namespace Firmware
