/**
 * @file gdt.hpp
 * @author Oguzhan KAYNAK ()
 * @brief Header file for Global Descriptor Table (GDT) structures and functions.
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

/**
 * @brief Namespace representing the Global Descriptor Table (GDT).
 *
 * The GDT namespace encapsulates the GDT table and its operations. It provides functions
 * for initializing the GDT, setting entries, and loading the GDT into the CPU.
 */
namespace GDT {

/// @brief Represents a GDT entry.
struct GDTEntry {
    uint16_t limitLow; /**< Limit 0:15 */
    uint16_t baseLow; /**< Base 0:15 */
    uint8_t baseMiddle; /**< Base 16:23 */
    uint8_t access; /**< empty (2 bit). */
    uint8_t limitHighAndFlags; /**< Limit 16:19 and Flags */
    uint8_t baseHigh; /**< Base 24:31 */
} __attribute__((packed));

/// @brief Represents the Global Descriptor Table Register (GDTR).
struct GDTR {
    uint16_t limit; ///< Size of the GDT - 1
    uintptr_t base; ///< Base address of the GDT
} __attribute__((packed));

/**
 * @brief The 64-bit Task State Segment (Intel SDM Vol. 3A, "Task Management in
 * 64-bit Mode"). In long mode it holds no task state any more, only stack
 * pointers: rsp0..2 for privilege changes and ist1..7 for IDT gates that ask
 * for a stack of their own.
 */
struct TaskStateSegment {
    uint32_t reserved0;
    uint64_t rsp[3]; ///< Stack for an interrupt that enters ring 0..2 from a lower privilege (user mode)
    uint64_t reserved1;
    uint64_t ist[7]; ///< Interrupt Stack Table: ist[0] is IST1, used by gates whose ist field is 1
    uint64_t reserved2;
    uint16_t reserved3;
    uint16_t ioMapBase; ///< Offset of the I/O permission bitmap; sizeof(TaskStateSegment) means "none"
} __attribute__((packed));

static_assert(sizeof(TaskStateSegment) == 104, "the CPU expects a 104 byte TSS");

/// @brief IST slot of the double fault handler (the ist field of IDT entry 8).
static constexpr uint8_t doubleFaultStack = 1;

/// @brief Size of the double fault stack.
static constexpr uint64_t doubleFaultStackSize = 16 * 1024ULL;

/**
 * @brief Points the TSS descriptor of the GDT at the TSS, gives IST1 its stack
 * and loads the task register (`ltr`). Call once, before IDT::init.
 */
void init();

/**
 * @brief Returns a reference to the GDTR.
 *
 * @return const GDTR&
 */
const GDTR& getGDTR();

/// @brief The TSS of this CPU (the only one).
const TaskStateSegment& taskStateSegment();

/**
 * @brief Set rsp0: the stack the CPU switches to when an interrupt, an
 * exception or a system call comes from ring 3. The scheduler sets it to the
 * running thread's kernel stack.
 */
void setKernelStack(uint64_t stackTop);

};
