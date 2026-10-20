/**
 * @file elf.cpp
 * @author Oguzhan KAYNAK
 * @brief Reading a static ELF64 executable.
 */

#include <proc/elf.hpp>

#include <string.h>

#include <mm/constants.hpp>
#include <mm/vmm.hpp>

namespace Process {

// The parts of the ELF64 file format the loader needs (System V ABI, "ELF-64 Object File Format")
struct ElfHeader {
    uint8_t ident[16]; // "\x7f" "ELF", class, data (endianness), version, ...
    uint16_t type; // 2: executable
    uint16_t machine; // 0x3E: x86_64
    uint32_t version;
    uint64_t entry;
    uint64_t programHeaderOffset;
    uint64_t sectionHeaderOffset;
    uint32_t flags;
    uint16_t headerSize;
    uint16_t programHeaderSize;
    uint16_t programHeaderCount;
    uint16_t sectionHeaderSize;
    uint16_t sectionHeaderCount;
    uint16_t sectionNameIndex;
} __attribute__((packed));

struct ProgramHeader {
    uint32_t type; // 1: PT_LOAD
    uint32_t flags; // PF_X 1, PF_W 2, PF_R 4
    uint64_t offset;
    uint64_t virtAddr;
    uint64_t physAddr;
    uint64_t fileSize;
    uint64_t memSize;
    uint64_t align;
} __attribute__((packed));

static_assert(sizeof(ElfHeader) == 64, "the ELF64 header is 64 bytes");
static_assert(sizeof(ProgramHeader) == 56, "an ELF64 program header is 56 bytes");

static constexpr uint8_t elfClass64 = 2;
static constexpr uint8_t elfDataLittleEndian = 1;
static constexpr uint16_t elfTypeExecutable = 2;
static constexpr uint16_t elfMachineX8664 = 0x3E;
static constexpr uint32_t programLoad = 1;
static constexpr uint32_t flagExecute = 1;
static constexpr uint32_t flagWrite = 2;

const char* elfErrorName(ElfError error)
{
    switch (error) {
    case ElfError::None:
        return "ok";
    case ElfError::TooSmall:
        return "too small for an ELF header";
    case ElfError::BadMagic:
        return "not an ELF file";
    case ElfError::NotElf64:
        return "not a 64-bit ELF file";
    case ElfError::NotLittleEndian:
        return "not little endian";
    case ElfError::NotExecutable:
        return "not an executable";
    case ElfError::WrongMachine:
        return "not for x86_64";
    case ElfError::BadProgramHeaders:
        return "broken program header table";
    case ElfError::NoSegments:
        return "nothing to load";
    case ElfError::TooManySegments:
        return "too many segments";
    case ElfError::SegmentOutsideFile:
        return "a segment goes past the end of the file";
    case ElfError::FileSizeAboveMemSize:
        return "a segment has more file bytes than memory";
    case ElfError::SegmentOutsideUserRange:
        return "a segment is outside the user address range";
    case ElfError::SegmentsSharePage:
        return "two segments share a 2 MB page (link with -z max-page-size=0x200000)";
    case ElfError::EntryOutsideSegments:
        return "the entry point is not in an executable segment";
    }
    return "?";
}

static uint64_t pageOf(uint64_t address)
{
    return address & ~(uint64_t)(Memory::pageSize2Mb - 1);
}

ElfError parseElf(const uint8_t* data, uint64_t size, ElfImage& image)
{
    if (size < sizeof(ElfHeader))
        return ElfError::TooSmall;

    ElfHeader header;
    memcpy(&header, data, sizeof(header));
    if (memcmp(header.ident, "\x7f"
                             "ELF",
            4)
        != 0)
        return ElfError::BadMagic;
    if (header.ident[4] != elfClass64)
        return ElfError::NotElf64;
    if (header.ident[5] != elfDataLittleEndian)
        return ElfError::NotLittleEndian;
    if (header.type != elfTypeExecutable)
        return ElfError::NotExecutable;
    if (header.machine != elfMachineX8664)
        return ElfError::WrongMachine;

    uint64_t tableSize = (uint64_t)header.programHeaderCount * sizeof(ProgramHeader);
    if (header.programHeaderSize != sizeof(ProgramHeader) || header.programHeaderOffset > size
        || tableSize > size - header.programHeaderOffset)
        return ElfError::BadProgramHeaders;

    image = {};
    image.entry = header.entry;
    for (uint32_t i = 0; i < header.programHeaderCount; i++) {
        ProgramHeader ph;
        memcpy(&ph, data + header.programHeaderOffset + (uint64_t)i * sizeof(ph), sizeof(ph));
        if (ph.type != programLoad || !ph.memSize)
            continue; // PT_NOTE, PT_GNU_STACK, ...: nothing to load

        if (image.segmentCount == ElfImage::maxSegments)
            return ElfError::TooManySegments;
        if (ph.offset > size || ph.fileSize > size - ph.offset)
            return ElfError::SegmentOutsideFile;
        if (ph.fileSize > ph.memSize)
            return ElfError::FileSizeAboveMemSize;
        if (ph.virtAddr < Memory::userBase || ph.virtAddr >= Memory::userEnd
            || ph.memSize > Memory::userEnd - ph.virtAddr)
            return ElfError::SegmentOutsideUserRange;

        image.segments[image.segmentCount++] = { ph.virtAddr, ph.memSize, ph.offset, ph.fileSize,
            (ph.flags & flagWrite) != 0, (ph.flags & flagExecute) != 0 };
    }
    if (!image.segmentCount)
        return ElfError::NoSegments;

    // Protection is per page: two segments may not share one
    for (uint32_t i = 0; i < image.segmentCount; i++) {
        const ElfSegment& a = image.segments[i];
        for (uint32_t j = i + 1; j < image.segmentCount; j++) {
            const ElfSegment& b = image.segments[j];
            if (pageOf(a.virtAddr) <= pageOf(b.virtAddr + b.memSize - 1)
                && pageOf(b.virtAddr) <= pageOf(a.virtAddr + a.memSize - 1))
                return ElfError::SegmentsSharePage;
        }
    }

    for (uint32_t i = 0; i < image.segmentCount; i++) {
        const ElfSegment& s = image.segments[i];
        if (s.executable && image.entry >= s.virtAddr && image.entry < s.virtAddr + s.memSize)
            return ElfError::None;
    }
    return ElfError::EntryOutsideSegments;
}

} // namespace Process
