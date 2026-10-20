/**
 * @file idt.hpp
 * @author Oguzhan KAYNAK
 * @brief Header file for Interrupt Descriptor Table(IDT)  structures and functions.
 *
 * Copyright © 2024 Oguzhan KAYNAK
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the “Software”), to deal in the Software
 * without  restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit
 * persons to whom the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
 * PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */
#pragma once

#include <stdint.h>

// for more info: https://wiki.osdev.org/Interrupts_Tutorial

/**
 * @brief ExternalInterrupts enum
 *
 * |--------|----------|---------------------------------------------|-----------|------------|------------------------------------|
 * | Vector | Mnemonic | Description                                 | Type      | Error Code | Source                             |
 * |--------|----------|---------------------------------------------|-----------|------------|------------------------------------|
 * | 0      | #DE      | Divide Error                                | Fault     | No         | DIV and IDIV instructions.         |
 * |--------|----------|---------------------------------------------|-----------|------------|------------------------------------|
 * | 1      | #DB      | Debug Exception                             | Fault/Trap| No         |Instruction, data, and I/O          |
 * |        |          |                                             |           |            |breakpoints; single-step; and others|
 * |--------|----------|---------------------------------------------|-----------|------------|------------------------------------|
 * | 2      | —        | NMI Interrupt                               | Interrupt | No         | Nonmaskable external interrupt.    |
 * |--------|----------|---------------------------------------------|-----------|------------|------------------------------------|
 * | 3      | #BP      | Breakpoint                                  | Trap      | No         | INT3 instruction.                  |
 * |--------|----------|---------------------------------------------|-----------|------------|------------------------------------|
 * | 4      | #OF      | Overflow                                    | Trap      | No         | INTO instruction.                  |
 * |--------|----------|---------------------------------------------|-----------|------------|------------------------------------|
 * | 5      | #BR      | BOUND Range Exceeded                        | Fault     | No         | BOUND instruction.                 |
 * |--------|----------|---------------------------------------------|-----------|------------|------------------------------------|
 * | 6      | #UD      | Invalid Opcode (Undefined Opcode)           | Fault     | No         | UD instruction or reserved opcode. |
 * |--------|----------|---------------------------------------------|-----------|------------|------------------------------------|
 * | 7      | #NM      | Device Not Available (No Math Coprocessor)  | Fault     | No         | Floating-point or                  |
 * |        |          |                                             |           |            | WAIT/FWAIT instruction.            |
 * |--------|----------|---------------------------------------------|-----------|------------|------------------------------------|
 * | 8      | #DF      | Double Fault                                | Abort     | Yes (zero) | Any instruction that can generate  |
 * |        |          |                                             |           |            | an exception, an NMI, or an INTR.  |
 * |--------|----------|---------------------------------------------|-----------|------------|------------------------------------|
 * | 9      | —        | Coprocessor Segment Overrun (reserved)      | Fault     | No         | Floating-point instruction.        |
 * |--------|----------|---------------------------------------------|-----------|------------|------------------------------------|
 * | 10     | #TS      | Invalid TSS                                 | Fault     | Yes        | Task switch or TSS access.         |
 * |--------|----------|---------------------------------------------|-----------|------------|------------------------------------|
 * | 11     | #NP      | Segment Not Present                         | Fault     | Yes        | Loading segment registers or       |
 * |        |          |                                             |           |            | accessing system segments.         |
 * |--------|----------|---------------------------------------------|-----------|------------|------------------------------------|
 * | 12     | #SS      | Stack-Segment Fault                         | Fault     | Yes        | Stack operations and               |
 * |        |          |                                             |           |            | SS register loads.                 |
 * |--------|----------|---------------------------------------------|-----------|------------|------------------------------------|
 * | 13     | #GP      | General Protection Fault                    | Fault     | Yes        | Any memory reference and           |
 * |        |          |                                             |           |            | other protection checks.           |
 * |--------|----------|---------------------------------------------|-----------|------------|------------------------------------|
 * | 14     | #PF      | Page Fault                                  | Fault     | Yes        | Any memory reference.              |
 * |--------|----------|---------------------------------------------|-----------|------------|------------------------------------|
 * | 15     | —        | (Intel reserved. Do not use.)               |           | No         |                                    |
 * |--------|----------|---------------------------------------------|-----------|------------|------------------------------------|
 * | 16     | #MF      | x87 FPU Floating-Point Error (Math Fault)   | Fault     | No         | x87 FPU floating-point or          |
 * |        |          |                                             |           |            |  WAIT/FWAIT instruction.           |
 * |--------|----------|---------------------------------------------|-----------|------------|------------------------------------|
 * | 17     | #AC      | Alignment Check                             | Fault     | Yes (zero) | Any data reference in memory.      |
 * |--------|----------|---------------------------------------------|-----------|------------|------------------------------------|
 * | 18     | #MC      | Machine Check                               | Abort     | No         | Error codes (if any) and source    |
 * |        |          |                                             |           |            | are model dependent.               |
 * |--------|----------|---------------------------------------------|-----------|------------|------------------------------------|
 * | 19     | #XM      | SIMD Floating-Point Exception               | Fault     | No         | SSE/SSE2/SSE3                      |
 * |        |          |                                             |           |            | floating-point instructions        |
 * |--------|----------|---------------------------------------------|-----------|------------|------------------------------------|
 * | 20     | #VE      | Virtualization Exception                    | Fault     | No         | EPT violations5                    |
 * |--------|----------|---------------------------------------------|-----------|------------|------------------------------------|
 * | 21     | #CP      | Control Protection Exception                | Fault     | Yes        | RET, IRET, RSTORSSP, and SETSSBSY  |
 * |        |          |                                             |           |            | instructions can generate          |
 * |        |          |                                             |           |            | this exception. When CET indirect  |
 * |        |          |                                             |           |            | branch tracking is enabled, this   |
 * |        |          |                                             |           |            | exception can be generated due to  |
 * |        |          |                                             |           |            | a missing ENDBRANCH instruction at |
 * |        |          |                                             |           |            | target of an indirect call or jump.|
 * |--------|----------|---------------------------------------------|-----------|------------|------------------------------------|
 * | 22-31  | —        | Intel reserved. Do not use.                 |           |            |                                    |
 * |--------|----------|---------------------------------------------|-----------|------------|------------------------------------|
 * | 32-255 | —        | User Defined (Non-reserved)                 | Interrupt |            | External interrupt or              |
 * |        |          |                                             |           |            | INT n instruction.                 |
 * |--------|----------|---------------------------------------------|-----------|------------|------------------------------------|
 *
 * For more information :
 *   Intel® 64 and IA-32 Architectures Software Developer’s Manual Combined Volumes: 1, 2A, 2B, 2C, 2D, 3A, 3B, 3C, 3D, and 4
 *   --> 6.3.1 External Interrupts
 */

namespace IDT {

enum class ExternalInterrupts : uint32_t {
    DivideError = 0,
    DebugException,
    NMIInterrupt,
    Breakpoint,
    Overflow,
    BOUNDRangeExceeded,
    InvalidOpcode,
    DeviceNotAvailable,
    DoubleFault,
    CoprocessorSegmentOverrun,
    InvalidTSS,
    SegmentNotPresent,
    StackSegmentFault,
    GeneralProtection,
    PageFault,
    IntelReserved,
    X87FpuFloatingPointError,
    AlignmentCheck,
    MachineCheck,
    SimdFloatingPointException,
    VirtualizationException,
    ControlProtectionException
};

/// @brief Structure representing an entry in the Interrupt Descriptor Table (IDT)
struct IDTEntry {
    enum TypeAttr : uint8_t {
        GateTask = 0x5,
        GateInt = 0xE, // 64-bit interrupt gate
        GateTrap = 0xF, // 64-bit trap gate

        RING0 = 0x00,
        RING1 = 0x20,
        RING2 = 0x40,
        RING3 = 0x60,

        PRESENT = 0x80,
    };

    uint16_t isrLow; ///< The lower 16 bits of the ISR's address
    uint16_t kernelCs; ///< The GDT segment selector that the CPU will load into CS before calling the ISR
    uint8_t ist; ///< IST slot (1..7) whose stack the CPU switches to, 0 = stay on the current stack
    uint8_t attributes; ///< Type and attributes; see the IDT page
    uint16_t isrMid; ///< The higher 16 bits of the lower 32 bits of the ISR's address
    uint32_t isrHigh; ///< The higher 32 bits of the ISR's address
    uint32_t reserved; ///< Set to zero

} __attribute__((packed));

/// @brief Structure representing the IDT Register (IDTR).
struct IDTR {
    uint16_t limit; ///< Size of the IDT (limit) minus 1
    uint64_t base; ///< Base address of the IDT
} __attribute__((packed));

static constexpr uint32_t idtTableLen = 256;

/// @brief The vector of system calls: "int $0x80", allowed from ring 3.
static constexpr uint32_t systemCallVector = 0x80;

/**
 * @brief Initializes the IDT.
 *
 */
void init();

/// @brief The IDT entry of a vector (read only, for the self tests).
const IDTEntry& entry(uint32_t vector);

}; // namespace IDT
