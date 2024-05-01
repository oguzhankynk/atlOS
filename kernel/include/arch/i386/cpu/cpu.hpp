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

#ifndef A046BEC9_DED9_4255_85EE_5065946448E0
#define A046BEC9_DED9_4255_85EE_5065946448E0

#include <stdint.h>
#include <stdio.h>

/// @brief Represents the Processor Type information.
struct ProcessorType
{
    uint32_t stepping : 4;       ///< Stepping (4 bits)
    uint32_t model : 4;          ///< Model (4 bits)
    uint32_t family : 4;         ///< Family (4 bits)
    uint32_t processorType : 2;  ///< Processor Type (2 bits)
    uint32_t reserved : 2;       ///< Reserved (2 bits)
    uint32_t extendedModel : 4;  ///< Extended Model (4 bits)
    uint32_t extendedFamily : 8; ///< Extended Family (8 bits)
};

/// @brief Represents the CPU features information.
struct CPUFeatures
{
    uint32_t SSE3 : 1;                ///< Streaming SIMD Extensions 3 (SSE3) support.
    uint32_t PCLMULQDQ : 1;           ///< PCLMULQDQ instruction support.
    uint32_t DTES64 : 1;              ///< 64-bit Debug Store Area support.
    uint32_t MONITOR : 1;             ///< MONITOR/MWAIT instruction support.
    uint32_t DS_CPL : 1;              ///< CPL Qualified Debug Store support.
    uint32_t VMX : 1;                 ///< Virtual Machine Extensions support.
    uint32_t SMX : 1;                 ///< Safer Mode Extensions support.
    uint32_t EIST : 1;                ///< Enhanced Intel SpeedStep Technology support.
    uint32_t TM2 : 1;                 ///< Thermal Monitor 2 support.
    uint32_t SSSE3 : 1;               ///< Supplemental Streaming SIMD Extensions 3 (SSSE3) support.
    uint32_t CNXT_ID : 1;             ///< L1 Context ID support.
    uint32_t SDBG : 1;                ///< Silicon Debug interface support.
    uint32_t FMA : 1;                 ///< Fused Multiply-Add (FMA) instruction support.
    uint32_t CMPXCHG16B : 1;          ///< CMPXCHG16B instruction support.
    uint32_t xTPR_Update_Control : 1; ///< xTPR Update Control support.
    uint32_t PDCM : 1;                ///< Perfmon and Debug Capability MSR support.
    uint32_t Reserved1 : 1;           ///< Reserved.
    uint32_t PCID : 1;                ///< Process-Context Identifiers (PCIDs) support.
    uint32_t DCA : 1;                 ///< Direct Cache Access (DCA) support.
    uint32_t SSE4_1 : 1;              ///< Streaming SIMD Extensions 4.1 (SSE4.1) support.
    uint32_t SSE4_2 : 1;              ///< Streaming SIMD Extensions 4.2 (SSE4.2) support.
    uint32_t x2APIC : 1;              ///< x2APIC support.
    uint32_t MOVBE : 1;               ///< MOVBE instruction support.
    uint32_t POPCNT : 1;              ///< POPCNT instruction support.
    uint32_t TSC_Deadline : 1;        ///< Time Stamp Counter Deadline (TSC-Deadline) support.
    uint32_t AESNI : 1;               ///< Advanced Encryption Standard (AES) instruction set support.
    uint32_t XSAVE : 1;               ///< XSAVE/XRSTOR/XSETBV/XGETBV instructions support.
    uint32_t OSXSAVE : 1;             ///< XSAVE-enabled extended states support.
    uint32_t AVX : 1;                 ///< Advanced Vector Extensions (AVX) support.
    uint32_t F16C : 1;                ///< 16-bit Floating-Point Conversion Instructions support.
    uint32_t RDRAND : 1;              ///< RDRAND instruction support.
    uint32_t Not_Used : 1;            ///< Not Used.
    uint32_t FPU : 1;                 ///< x87 FPU on-chip support.
    uint32_t VME : 1;                 ///< Virtual 8086 Mode Enhancements (VME) support.
    uint32_t DE : 1;                  ///< Debugging Extensions (DE) support.
    uint32_t PSE : 1;                 ///< Page Size Extensions (PSE) support.
    uint32_t TSC : 1;                 ///< Time Stamp Counter (TSC) support.
    uint32_t MSR : 1;                 ///< Model-Specific Registers (MSR) support.
    uint32_t PAE : 1;                 ///< Physical Address Extensions (PAE) support.
    uint32_t MCE : 1;                 ///< Machine-Check Exception (MCE) support.
    uint32_t CX8 : 1;                 ///< CMPXCHG8B instruction support.
    uint32_t APIC : 1;                ///< Advanced Programmable Interrupt Controller (APIC) support.
    uint32_t Reserved2 : 1;           ///< Reserved.
    uint32_t SEP : 1;                 ///< SYSENTER/SYSEXIT instructions support.
    uint32_t MTRR : 1;                ///< Memory Type Range Registers (MTRR) support.
    uint32_t PGE : 1;                 ///< Page Global Enable (PGE) support.
    uint32_t MCA : 1;                 ///< Machine-Check Architecture (MCA) support.
    uint32_t CMOV : 1;                ///< Conditional Move (CMOV) support.
    uint32_t PAT : 1;                 ///< Page Attribute Table (PAT) support.
    uint32_t PSE_36 : 1;              ///< 36-bit Page Size Extensions (PSE-36) support.
    uint32_t PSN : 1;                 ///< Processor Serial Number (PSN) support.
    uint32_t CLFSH : 1;               ///< CLFLUSH instruction support.
    uint32_t Reserved3 : 1;           ///< Reserved.
    uint32_t DS : 1;                  ///< Debug Store (DS) support.
    uint32_t ACPI : 1;                ///< ACPI support.
    uint32_t MMX : 1;                 ///< Multi-Media Extensions (MMX) support.
    uint32_t FXSR : 1;                ///< FXSAVE/FXRSTOR instruction support.
    uint32_t SSE : 1;                 ///< Streaming SIMD Extensions (SSE) support.
    uint32_t SSE2 : 1;                ///< Streaming SIMD Extensions 2 (SSE2) support.
    uint32_t SS : 1;                  ///< Self-Snoop (SS) support.
    uint32_t HTT : 1;                 ///< Hyper-Threading Technology (HTT) support.
    uint32_t TM : 1;                  ///< Thermal Monitor (TM) support.
    uint32_t Reserved4 : 1;           ///< Reserved.
    uint32_t PBE : 1;                 ///< Pending Break Enable (PBE) support.
};

/// @brief Represents the CPU topology information.
struct CPUTopology
{
    uint32_t logical_CPU_bits; ///< Number of bits representing logical CPUs.
    uint32_t core_bits;        ///< Number of bits representing CPU cores.
};

/// @brief Represents the cache information.
struct CacheInfo
{
    uint32_t cacheType : 5;               ///< Cache type (5 bits).
    uint32_t cacheLevel : 3;              ///< Cache level (3 bits).
    uint32_t selfInitializing : 1;        ///< Self-initializing cache flag (1 bit).
    uint32_t fullyAssociative : 1;        ///< Fully associative cache flag (1 bit).
    uint32_t LogicalCore : 12;            ///< Number of logical cores (12 bits).
    uint32_t PhysicalCore : 6;            ///< Number of physical cores (6 bits).
    uint32_t coherencyLineSize : 12;      ///< Coherency line size (12 bits).
    uint32_t physicalLinePartitions : 10; ///< Number of physical line partitions (10 bits).
    uint32_t associativity : 10;          ///< Cache associativity (10 bits).
    uint32_t numberOfSets : 32;           ///< Number of cache sets (32 bits).
    uint32_t writeBackInvalidate : 1;     ///< Write-back invalidate flag (1 bit).
    uint32_t cacheInclusiveness : 1;      ///< Cache inclusiveness flag (1 bit).
    uint32_t complexCacheIndexing : 1;    ///< Complex cache indexing flag (1 bit).
};

/// @brief Represents a list of caches.
struct CacheList
{
    uint8_t cacheCount;     ///< Number of caches in the list.
    CacheInfo cacheInfo[8]; ///< Array of cache information structures.
};

/// @brief Represents CPU information.
struct CPU_Info
{
    char cpu[32];                ///< CPU model information (32 characters).
    uint32_t maksimumIndex;      ///< Maximum index.
    CPUFeatures CPUFeaturesInfo; ///< CPU features information.
    ProcessorType processorType; ///< Processor type information.
    CPUTopology CPUTopologyInfo; ///< CPU topology information.
    CacheList cacheList;         ///< List of caches.
};

/// @brief Enumeration for segment selectors.
enum class SegmentSelect : uint16_t
{
    Segment_NULL = 0x00, ///< Null segment selector.

    KERNELCODE = 0x08, ///< Kernel code segment selector.

    KERNELDATA = 0x10, ///< Kernel data segment selector.

    USERCODE = 0x23, ///< User code segment selector.

    USERDATA = 0x1b, ///< User data segment selector.

    TSS = 0x28 ///< Task State Segment (TSS) selector.
};

/// @brief Structure representing the state of x86 registers at exception time.
struct X86ExceptionInfo
{
    uint32_t edi; ///< Extended Destination Index register
    uint32_t esi; ///< Extended Source Index register
    uint32_t ebp; ///< Extended Base Pointer register
    uint32_t esp; ///< Extended Stack Pointer register
    uint32_t ebx; ///< Extended Base register
    uint32_t edx; ///< Extended Data register
    uint32_t ecx; ///< Extended Counter register
    uint32_t eax; ///< Extended Accumulator register

    uint32_t int_no;   ///< Interrupt number or exception code
    uint32_t err_code; ///< Error code (if applicable)

    /**
     * @brief print register's status.
     *
     */
    void print()
    {
        printf("REGISTERS:\n");
        printf("err_code=%u int_no=%u \n", err_code, int_no);
        printf("eax=0x%x, ebx=0x%x, ecx=0x%x, edx=0x%x\n", eax, ebx, ecx, edx);
        printf("edi=0x%x, esi=0x%x, ebp=0x%x, esp=0x%x\n", edi, esi, ebp, esp);
    }
};

/// @brief Initializes the CPU information structure.
/// @param cpu_info Pointer to the CPU_Info structure to be initialized.
void InitializeCPUInfo(CPU_Info *cpu_info);

/// @brief Prints the CPU information.
/// @param cpu_info Pointer to the CPU_Info structure containing CPU information to be printed.
void printCPUInfo(const CPU_Info *cpu_info);

#endif /* A046BEC9_DED9_4255_85EE_5065946448E0 */
