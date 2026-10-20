/**
 * @file fault.hpp
 * @author Oguzhan KAYNAK
 * @brief Decoding and reporting of CPU exceptions (vectors 0..31).
 *
 * The decode functions only take the error code the CPU pushed, so the self
 * tests can check them without causing a fault.
 */

#pragma once

#include <stdint.h>

#include <arch/x86_64/cpu/cpu.hpp>

namespace Fault {

/// @brief Error code of a page fault (Intel SDM Vol. 3A, "Interrupt 14—Page-Fault Exception").
struct PageFaultError {
    bool present; ///< false: the page is not present, true: a protection violation on a present page
    bool write; ///< false: a read, true: a write
    bool user; ///< the access came from ring 3
    bool reservedBit; ///< a page table entry has a reserved bit set
    bool instructionFetch; ///< the access was an instruction fetch
};

PageFaultError decodePageFault(uint64_t errorCode);

/// @brief The descriptor table a selector error code points into.
enum class DescriptorTable {
    GDT,
    IDT,
    LDT,
};

/**
 * @brief Selector error code of #TS, #NP, #SS and #GP (Intel SDM Vol. 3A,
 * "Error Code"). An error code of 0 means the fault was not caused by a selector.
 */
struct SelectorError {
    bool external; ///< raised while delivering an external event (an interrupt)
    DescriptorTable table;
    uint16_t index; ///< descriptor index (for the IDT: the vector)
};

SelectorError decodeSelectorError(uint64_t errorCode);

/// @brief Name of an exception vector, nullptr for vectors 32..255.
const char* exceptionName(uint64_t vector);

/**
 * @brief Print what the CPU reported (vector, decoded error code, CR2 for a
 * page fault, registers) and panic with a backtrace of the faulting code.
 */
[[noreturn]] void reportException(const InterruptFrame& frame);

} // namespace Fault
