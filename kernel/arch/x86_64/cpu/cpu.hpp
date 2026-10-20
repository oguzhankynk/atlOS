/**
 * @file cpu.hpp
 * @author Oguzhan KAYNAK
 * @brief Header file containing CPU related function declarations and structures.
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

#define MSR_EFER (0xC0000080)
#define MSR_EFER_LME (1 << 8)

#ifndef __ASSEMBLER__

#include <stdint.h>
#include <stdio.h>

/// @brief Represents the Processor Type information.
struct ProcessorType {
    uint32_t stepping : 4; ///< Stepping (4 bits)
    uint32_t model : 4; ///< Model (4 bits)
    uint32_t family : 4; ///< Family (4 bits)
    uint32_t processorType : 2; ///< Processor Type (2 bits)
    uint32_t reserved : 2; ///< Reserved (2 bits)
    uint32_t extendedModel : 4; ///< Extended Model (4 bits)
    uint32_t extendedFamily : 8; ///< Extended Family (8 bits)
};

/// @brief Represents the CPU features information.
struct CPUFeatures {
    uint32_t sse3 : 1; ///< Streaming SIMD Extensions 3 (SSE3) support.
    uint32_t pclmulqdq : 1; ///< PCLMULQDQ instruction support.
    uint32_t dtes64 : 1; ///< 64-bit Debug Store Area support.
    uint32_t monitor : 1; ///< MONITOR/MWAIT instruction support.
    uint32_t dsCpl : 1; ///< CPL Qualified Debug Store support.
    uint32_t vmx : 1; ///< Virtual Machine Extensions support.
    uint32_t smx : 1; ///< Safer Mode Extensions support.
    uint32_t eist : 1; ///< Enhanced Intel SpeedStep Technology support.
    uint32_t tm2 : 1; ///< Thermal Monitor 2 support.
    uint32_t ssse3 : 1; ///< Supplemental Streaming SIMD Extensions 3 (SSSE3) support.
    uint32_t cnxtId : 1; ///< L1 Context ID support.
    uint32_t sdbg : 1; ///< Silicon Debug interface support.
    uint32_t fma : 1; ///< Fused Multiply-Add (FMA) instruction support.
    uint32_t cmpxchg16b : 1; ///< CMPXCHG16B instruction support.
    uint32_t xTprUpdateControl : 1; ///< xTPR Update Control support.
    uint32_t pdcm : 1; ///< Perfmon and Debug Capability MSR support.
    uint32_t reserved1 : 1; ///< Reserved.
    uint32_t pcid : 1; ///< Process-Context Identifiers (PCIDs) support.
    uint32_t dca : 1; ///< Direct Cache Access (DCA) support.
    uint32_t sse41 : 1; ///< Streaming SIMD Extensions 4.1 (SSE4.1) support.
    uint32_t sse42 : 1; ///< Streaming SIMD Extensions 4.2 (SSE4.2) support.
    uint32_t x2APIC : 1; ///< x2APIC support.
    uint32_t movbe : 1; ///< MOVBE instruction support.
    uint32_t popcnt : 1; ///< POPCNT instruction support.
    uint32_t tscDeadline : 1; ///< Time Stamp Counter Deadline (TSC-Deadline) support.
    uint32_t aesni : 1; ///< Advanced Encryption Standard (AES) instruction set support.
    uint32_t xsave : 1; ///< XSAVE/XRSTOR/XSETBV/XGETBV instructions support.
    uint32_t osxsave : 1; ///< XSAVE-enabled extended states support.
    uint32_t avx : 1; ///< Advanced Vector Extensions (AVX) support.
    uint32_t f16c : 1; ///< 16-bit Floating-Point Conversion Instructions support.
    uint32_t rdrand : 1; ///< RDRAND instruction support.
    uint32_t notUsed : 1; ///< Not Used.
    uint32_t fpu : 1; ///< x87 FPU on-chip support.
    uint32_t vme : 1; ///< Virtual 8086 Mode Enhancements (VME) support.
    uint32_t de : 1; ///< Debugging Extensions (DE) support.
    uint32_t pse : 1; ///< Page Size Extensions (PSE) support.
    uint32_t tsc : 1; ///< Time Stamp Counter (TSC) support.
    uint32_t msr : 1; ///< Model-Specific Registers (MSR) support.
    uint32_t pae : 1; ///< Physical Address Extensions (PAE) support.
    uint32_t mce : 1; ///< Machine-Check Exception (MCE) support.
    uint32_t cx8 : 1; ///< CMPXCHG8B instruction support.
    uint32_t apic : 1; ///< Advanced Programmable Interrupt Controller (APIC) support.
    uint32_t reserved2 : 1; ///< Reserved.
    uint32_t sep : 1; ///< SYSENTER/SYSEXIT instructions support.
    uint32_t mtrr : 1; ///< Memory Type Range Registers (MTRR) support.
    uint32_t pge : 1; ///< Page Global Enable (PGE) support.
    uint32_t mca : 1; ///< Machine-Check Architecture (MCA) support.
    uint32_t cmov : 1; ///< Conditional Move (CMOV) support.
    uint32_t pat : 1; ///< Page Attribute Table (PAT) support.
    uint32_t pse36 : 1; ///< 36-bit Page Size Extensions (PSE-36) support.
    uint32_t psn : 1; ///< Processor Serial Number (PSN) support.
    uint32_t clfsh : 1; ///< CLFLUSH instruction support.
    uint32_t reserved3 : 1; ///< Reserved.
    uint32_t ds : 1; ///< Debug Store (DS) support.
    uint32_t acpi : 1; ///< ACPI support.
    uint32_t mmx : 1; ///< Multi-Media Extensions (MMX) support.
    uint32_t fxsr : 1; ///< FXSAVE/FXRSTOR instruction support.
    uint32_t sse : 1; ///< Streaming SIMD Extensions (SSE) support.
    uint32_t sse2 : 1; ///< Streaming SIMD Extensions 2 (SSE2) support.
    uint32_t ss : 1; ///< Self-Snoop (SS) support.
    uint32_t htt : 1; ///< Hyper-Threading Technology (HTT) support.
    uint32_t tm : 1; ///< Thermal Monitor (TM) support.
    uint32_t reserved4 : 1; ///< Reserved.
    uint32_t pbe : 1; ///< Pending Break Enable (PBE) support.
};

/// @brief Represents the CPU topology information.
struct CPUTopology {
    uint32_t logicalCpuBits; ///< Number of bits representing logical CPUs.
    uint32_t coreBits; ///< Number of bits representing CPU cores.
};

/// @brief Represents the cache information.
struct CacheInfo {
    uint32_t cacheType : 5; ///< Cache type (5 bits).
    uint32_t cacheLevel : 3; ///< Cache level (3 bits).
    uint32_t selfInitializing : 1; ///< Self-initializing cache flag (1 bit).
    uint32_t fullyAssociative : 1; ///< Fully associative cache flag (1 bit).
    uint32_t logicalCore : 12; ///< Number of logical cores (12 bits).
    uint32_t physicalCore : 6; ///< Number of physical cores (6 bits).
    uint32_t coherencyLineSize : 12; ///< Coherency line size (12 bits).
    uint32_t physicalLinePartitions : 10; ///< Number of physical line partitions (10 bits).
    uint32_t associativity : 10; ///< Cache associativity (10 bits).
    uint32_t numberOfSets : 32; ///< Number of cache sets (32 bits).
    uint32_t writeBackInvalidate : 1; ///< Write-back invalidate flag (1 bit).
    uint32_t cacheInclusiveness : 1; ///< Cache inclusiveness flag (1 bit).
    uint32_t complexCacheIndexing : 1; ///< Complex cache indexing flag (1 bit).
};

/// @brief Represents a list of caches.
struct CacheList {
    uint8_t cacheCount; ///< Number of caches in the list.
    CacheInfo cacheInfo[8]; ///< Array of cache information structures.
};

/// @brief Represents CPU information.
struct CPUInfo {
    char cpu[32]; ///< CPU model information (32 characters).
    uint32_t maksimumIndex; ///< Maximum index.
    CPUFeatures cpuFeaturesInfo; ///< CPU features information.
    ProcessorType processorType; ///< Processor type information.
    CPUTopology cpuTopologyInfo; ///< CPU topology information.
    CacheList cacheList; ///< List of caches.
};

/// @brief Enumeration for segment selectors.
enum class SegmentSelect : uint16_t {
    SegmentNull = 0x00, ///< Null segment selector.

    KERNELCODE = 0x08, ///< Kernel code segment selector.

    KERNELDATA = 0x10, ///< Kernel data segment selector.

    USERCODE = 0x1b, ///< User code segment selector.

    USERDATA = 0x23, ///< User data segment selector.

    TSS = 0x28 ///< Task State Segment (TSS) selector.
};

/// @brief Structure representing the state of x86_64 registers at exception/interrupt time.
struct InterruptFrame {
    // General-purpose registers (64-bit)
    uint64_t r15;
    uint64_t r14;
    uint64_t r13;
    uint64_t r12;
    uint64_t r11;
    uint64_t r10;
    uint64_t r9;
    uint64_t r8;
    uint64_t rdi; // isr.S pushes rsi, then rdi: rdi is at the lower address
    uint64_t rsi;
    uint64_t rbp;
    uint64_t rdx;
    uint64_t rcx;
    uint64_t rbx;
    uint64_t rax;

    // Interrupt number and error code
    uint64_t intNo;
    uint64_t errCode;

    // Pushed automatically by CPU on interrupt/exception
    uint64_t rip;
    uint64_t cs;
    uint64_t rflags;
    uint64_t rsp;
    uint64_t ss;
} __attribute__((packed));

/// @brief Initializes the CPU information structure.
/// @param cpuInfo Pointer to the CPUInfo structure to be initialized.
void initializeCpuInfo(CPUInfo* cpuInfo);

/// @brief Prints the CPU information.
/// @param cpuInfo Pointer to the CPUInfo structure containing CPU information to be printed.
void printCPUInfo(const CPUInfo* cpuInfo);

#endif // __ASSEMBLER__
