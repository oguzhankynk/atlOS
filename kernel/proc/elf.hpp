/**
 * @file elf.hpp
 * @author Oguzhan KAYNAK
 * @brief Reading a static ELF64 executable: where its segments go and where
 * it starts. Only checks and copies numbers out of the file, so the self
 * tests can feed it made up headers.
 */

#pragma once

#include <stdint.h>

namespace Process {

/// One PT_LOAD segment: memSize bytes at virtAddr, the first fileSize of them from the file
struct ElfSegment {
    uint64_t virtAddr;
    uint64_t memSize;
    uint64_t fileOffset;
    uint64_t fileSize;
    bool writable;
    bool executable;
};

struct ElfImage {
    static constexpr uint32_t maxSegments = 8;

    uint64_t entry;
    ElfSegment segments[maxSegments];
    uint32_t segmentCount;
};

enum class ElfError {
    None,
    TooSmall, ///< shorter than an ELF header
    BadMagic, ///< not "\x7f" "ELF"
    NotElf64, ///< 32-bit ELF
    NotLittleEndian,
    NotExecutable, ///< a relocatable object or shared library (atlOS loads static executables)
    WrongMachine, ///< not x86_64
    BadProgramHeaders, ///< the program header table does not fit the file
    NoSegments,
    TooManySegments,
    SegmentOutsideFile, ///< its file bytes go past the end of the file
    FileSizeAboveMemSize,
    SegmentOutsideUserRange, ///< not inside [userBase, userEnd)
    SegmentsSharePage, ///< two segments in one 2 MB page (link with -z max-page-size=0x200000)
    EntryOutsideSegments, ///< the entry point is not in an executable segment
};

const char* elfErrorName(ElfError error);

/// Check the file and collect its PT_LOAD segments and entry point.
ElfError parseElf(const uint8_t* data, uint64_t size, ElfImage& image);

} // namespace Process
