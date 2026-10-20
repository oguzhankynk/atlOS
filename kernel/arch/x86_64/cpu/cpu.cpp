/**
 * @file cpu.cpp
 * @author Oguzhan KAYNAK
 * @brief Functions for CPU management and control.
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
 *
 */
#include <arch/x86_64/cpu/cpu.hpp>
#include <arch/x86_64/cpu/cpuid.hpp>
#include <stdio.h>
#include <string.h>

static void printCPUFeatures(const CPUFeatures* features);
static void printProcessorType(const ProcessorType* processor);
static void printCacheInfo(const CacheList* cacheList);

static void cpuid00hCommand(uint32_t* pMaximumIndex, char* cpuString);
static void cpuid01hCommand(ProcessorType* pProcessorType, CPUFeatures* pCpuFeatures);
static void cpuid04hCommand(CacheList* pCacheList);

void initializeCpuInfo(CPUInfo* cpuInfo)
{
    // Fields a leaf does not fill (e.g. the topology) stay zero
    memset(cpuInfo, 0, sizeof(*cpuInfo));

    cpuid00hCommand(&cpuInfo->maksimumIndex, cpuInfo->cpu);

    if (cpuInfo->maksimumIndex >= 0x01)
        cpuid01hCommand(&cpuInfo->processorType, &cpuInfo->cpuFeaturesInfo);

    // Above the highest supported leaf CPUID returns unrelated data
    if (cpuInfo->maksimumIndex >= 0x04)
        cpuid04hCommand(&cpuInfo->cacheList);
}

void printCPUInfo(const CPUInfo* cpuInfo)
{
    printf("CPU: %s\n", cpuInfo->cpu);
    printf("Maksimum Index: %u\n", cpuInfo->maksimumIndex);
    printCPUFeatures(&cpuInfo->cpuFeaturesInfo);
    printf("\n");
    printProcessorType(&cpuInfo->processorType);
    printCacheInfo(&cpuInfo->cacheList);
}

/// @brief Prints the CPU features.
/// @param features: A pointer to CPU features to print.
static void printCPUFeatures(const CPUFeatures* features)
{
    printf("SSE3: %u\t", features->sse3);
    printf("PCLMULQDQ: %u\t", features->pclmulqdq);
    printf("DTES64: %u\t", features->dtes64);
    printf("MONITOR: %u\t", features->monitor);
    printf("DS_CPL: %u\t", features->dsCpl);
    printf("VMX: %u\t", features->vmx);
    printf("SMX: %u\t", features->smx);
    printf("EIST: %u\t", features->eist);
    printf("TM2: %u\t", features->tm2);
    printf("SSSE3: %u\t", features->ssse3);
    printf("CNXT_ID: %u\t", features->cnxtId);
    printf("SDBG: %u\t", features->sdbg);
    printf("FMA: %u\t", features->fma);
    printf("CMPXCHG16B: %u\t", features->cmpxchg16b);
    printf("xTPR_Update_Control: %u\t", features->xTprUpdateControl);
    printf("PDCM: %u\t", features->pdcm);
    printf("Reserved1: %u\t", features->reserved1);
    printf("PCID: %u\t", features->pcid);
    printf("DCA: %u\t", features->dca);
    printf("SSE4_1: %u\t", features->sse41);
    printf("SSE4_2: %u\t", features->sse42);
    printf("x2APIC: %u\t", features->x2APIC);
    printf("MOVBE: %u\t", features->movbe);
    printf("POPCNT: %u\t", features->popcnt);
    printf("TSC_Deadline: %u\t", features->tscDeadline);
    printf("AESNI: %u\t", features->aesni);
    printf("XSAVE: %u\t", features->xsave);
    printf("OSXSAVE: %u\t", features->osxsave);
    printf("AVX: %u\t", features->avx);
    printf("F16C: %u\t", features->f16c);
    printf("RDRAND: %u\t", features->rdrand);
    printf("Not_Used: %u\t", features->notUsed);
    printf("FPU: %u\t", features->fpu);
    printf("VME: %u\t", features->vme);
    printf("DE: %u\t", features->de);
    printf("PSE: %u\t", features->pse);
    printf("TSC: %u\t", features->tsc);
    printf("MSR: %u\t", features->msr);
    printf("PAE: %u\t", features->pae);
    printf("MCE: %u\t", features->mce);
    printf("CX8: %u\t", features->cx8);
    printf("APIC: %u\t", features->apic);
    printf("Reserved2: %u\t", features->reserved2);
    printf("SEP: %u\t", features->sep);
    printf("MTRR: %u\t", features->mtrr);
    printf("PGE: %u\t", features->pge);
    printf("MCA: %u\t", features->mca);
    printf("CMOV: %u\t", features->cmov);
    printf("PAT: %u\t", features->pat);
    printf("PSE_36: %u\t", features->pse36);
    printf("PSN: %u\t", features->psn);
    printf("CLFSH: %u\t", features->clfsh);
    printf("Reserved3: %u\t", features->reserved3);

    printf("DS : %u\t", features->ds);
    printf("ACPI : %u\t", features->acpi);
    printf("MMX : %u\t", features->mmx);
    printf("FXSR : %u\t", features->fxsr);
    printf("SSE : %u\t", features->sse);
    printf("SSE2 : %u\t", features->sse2);
    printf("SS : %u\t", features->ss);
    printf("HTT : %u\t", features->htt);
    printf("TM : %u\t", features->tm);
    printf("Reserved4 : %u\t", features->reserved4);
    printf("PBE : %u\t", features->pbe);
}

/// @brief Prints the processor type.
/// @param processor: A pointer to the processor type information to print.
static void printProcessorType(const ProcessorType* processor)
{
    printf("Stepping: %u\n", processor->stepping);
    printf("Model: %u\n", processor->model);
    printf("Family: %u\n", processor->family);
    printf("Processor Type: %u\n", processor->processorType);
    printf("Reserved: %u\n", processor->reserved);
    printf("Extended Model: %u\n", processor->extendedModel);
    printf("Extended Family: %u\n", processor->extendedFamily);
}

/// @brief Prints the cache information.
/// @param cacheList: A pointer to the cache information to print.
static void printCacheInfo(const CacheList* pCacheList)
{
    const char* cacheTypeStrs[] = {
        "Null",
        "D-Cache",
        "I-Cache",
        "Unified",
    };

    for (int i = 0; i < pCacheList->cacheCount; i++) {
        const CacheInfo* pCacheInfo = &pCacheList->cacheInfo[i];

        printf("Cache [%d]: \n", i);
        printf("cacheType: %u -> %s cacheLevel: %u self: %u \n",
            pCacheInfo->cacheType, cacheTypeStrs[pCacheInfo->cacheType],
            pCacheInfo->cacheLevel, pCacheInfo->selfInitializing);

        printf("fully: %u LogicalCore: %u Cores: %u\n",
            pCacheInfo->fullyAssociative, pCacheInfo->logicalCore,
            pCacheInfo->physicalCore);

        printf("LineSize: %u Partitions: %u associativity: %u\n",
            pCacheInfo->coherencyLineSize, pCacheInfo->physicalLinePartitions,
            pCacheInfo->associativity);

        printf("numberOfSets: %u \n", pCacheInfo->numberOfSets);

        printf("writeBackInvalidate: %u cacheInclusiveness: %u complexCacheIndexing:%u \n",
            pCacheInfo->writeBackInvalidate, pCacheInfo->cacheInclusiveness, pCacheInfo->complexCacheIndexing);

        printf("\n");
    }
}

/// @brief Executes the CPUID command with EAX=0x00 and stores the maximum index and CPU string.
/// @param pMaximumIndex: A pointer to store the maximum index.
/// @param cpuString: A pointer to store the CPU string.
static void cpuid00hCommand(uint32_t* pMaximumIndex, char* cpuString)
{
    unsigned int eax, ebx, ecx, edx;
    cpuid(0x00, &eax, &ebx, &ecx, &edx);

    sprintf(cpuString, "%c%c%c%c", ebx, (ebx >> 8), (ebx >> 16), (ebx >> 24));
    sprintf(cpuString + 4, "%c%c%c%c", edx, (edx >> 8), (edx >> 16), (edx >> 24));
    sprintf(cpuString + 8, "%c%c%c%c", ecx, (ecx >> 8), (ecx >> 16), (ecx >> 24));

    *pMaximumIndex = eax;
}

/// @brief Executes the CPUID command with EAX=0x01 and retrieves processor type and CPU features.
/// @param pProcessorType Pointer to a ProcessorType structure where the processor type information will be stored.
/// @param pCpuFeatures Pointer to a CPUFeatures structure where the CPU feature information will be stored.
static void cpuid01hCommand(ProcessorType* pProcessorType, CPUFeatures* pCpuFeatures)

{
    unsigned int eax, ebx, ecx, edx;
    cpuid(0x01, &eax, &ebx, &ecx, &edx);

    pProcessorType->stepping = eax & 0xF;
    pProcessorType->model = (eax >> 4) & 0xF;
    pProcessorType->family = (eax >> 8) & 0xF;
    pProcessorType->processorType = (eax >> 12) & 0x3;
    pProcessorType->reserved = (eax >> 14) & 0x3;
    pProcessorType->extendedModel = (eax >> 16) & 0xF;
    pProcessorType->extendedFamily = (eax >> 20) & 0xFF;

    pCpuFeatures->sse3 = (ecx & (1 << 0)) != 0;
    pCpuFeatures->pclmulqdq = (ecx & (1 << 1)) != 0;
    pCpuFeatures->dtes64 = (ecx & (1 << 2)) != 0;
    pCpuFeatures->monitor = (ecx & (1 << 3)) != 0;
    pCpuFeatures->dsCpl = (ecx & (1 << 4)) != 0;
    pCpuFeatures->vmx = (ecx & (1 << 5)) != 0;
    pCpuFeatures->smx = (ecx & (1 << 6)) != 0;
    pCpuFeatures->eist = (ecx & (1 << 7)) != 0;
    pCpuFeatures->tm2 = (ecx & (1 << 8)) != 0;
    pCpuFeatures->ssse3 = (ecx & (1 << 9)) != 0;
    pCpuFeatures->cnxtId = (ecx & (1 << 10)) != 0;
    pCpuFeatures->sdbg = (ecx & (1 << 11)) != 0;
    pCpuFeatures->fma = (ecx & (1 << 12)) != 0;
    pCpuFeatures->cmpxchg16b = (ecx & (1 << 13)) != 0;
    pCpuFeatures->xTprUpdateControl = (ecx & (1 << 14)) != 0;
    pCpuFeatures->pdcm = (ecx & (1 << 15)) != 0;
    pCpuFeatures->reserved1 = (ecx & (1 << 16)) != 0;
    pCpuFeatures->pcid = (ecx & (1 << 17)) != 0;
    pCpuFeatures->dca = (ecx & (1 << 18)) != 0;
    pCpuFeatures->sse41 = (ecx & (1 << 19)) != 0;
    pCpuFeatures->sse42 = (ecx & (1 << 20)) != 0;
    pCpuFeatures->x2APIC = (ecx & (1 << 21)) != 0;
    pCpuFeatures->movbe = (ecx & (1 << 22)) != 0;
    pCpuFeatures->popcnt = (ecx & (1 << 23)) != 0;
    pCpuFeatures->tscDeadline = (ecx & (1 << 24)) != 0;
    pCpuFeatures->aesni = (ecx & (1 << 25)) != 0;
    pCpuFeatures->xsave = (ecx & (1 << 26)) != 0;
    pCpuFeatures->osxsave = (ecx & (1 << 27)) != 0;
    pCpuFeatures->avx = (ecx & (1 << 28)) != 0;
    pCpuFeatures->f16c = (ecx & (1 << 29)) != 0;
    pCpuFeatures->rdrand = (ecx & (1 << 30)) != 0;
    pCpuFeatures->notUsed = (ecx & (1 << 31)) != 0;

    pCpuFeatures->fpu = (edx & (1 << 0)) != 0;
    pCpuFeatures->vme = (edx & (1 << 1)) != 0;
    pCpuFeatures->de = (edx & (1 << 2)) != 0;
    pCpuFeatures->pse = (edx & (1 << 3)) != 0;
    pCpuFeatures->tsc = (edx & (1 << 4)) != 0;
    pCpuFeatures->msr = (edx & (1 << 5)) != 0;
    pCpuFeatures->pae = (edx & (1 << 6)) != 0;
    pCpuFeatures->mce = (edx & (1 << 7)) != 0;
    pCpuFeatures->cx8 = (edx & (1 << 8)) != 0;
    pCpuFeatures->apic = (edx & (1 << 9)) != 0;
    pCpuFeatures->reserved2 = (edx & (1 << 10)) != 0;
    pCpuFeatures->sep = (edx & (1 << 11)) != 0;
    pCpuFeatures->mtrr = (edx & (1 << 12)) != 0;
    pCpuFeatures->pge = (edx & (1 << 13)) != 0;
    pCpuFeatures->mca = (edx & (1 << 14)) != 0;
    pCpuFeatures->cmov = (edx & (1 << 15)) != 0;
    pCpuFeatures->pat = (edx & (1 << 16)) != 0;
    pCpuFeatures->pse36 = (edx & (1 << 17)) != 0;
    pCpuFeatures->psn = (edx & (1 << 18)) != 0;
    pCpuFeatures->clfsh = (edx & (1 << 19)) != 0;
    pCpuFeatures->reserved3 = (edx & (1 << 20)) != 0;
    pCpuFeatures->ds = (edx & (1 << 21)) != 0;
    pCpuFeatures->acpi = (edx & (1 << 22)) != 0;
    pCpuFeatures->mmx = (edx & (1 << 23)) != 0;
    pCpuFeatures->fxsr = (edx & (1 << 24)) != 0;
    pCpuFeatures->sse = (edx & (1 << 25)) != 0;
    pCpuFeatures->sse2 = (edx & (1 << 26)) != 0;
    pCpuFeatures->ss = (edx & (1 << 27)) != 0;
    pCpuFeatures->htt = (edx & (1 << 28)) != 0;
    pCpuFeatures->tm = (edx & (1 << 29)) != 0;
    pCpuFeatures->reserved4 = (edx & (1 << 30)) != 0;
    pCpuFeatures->pbe = (edx & (1 << 31)) != 0;
}

/// @brief Executes the CPUID command with EAX=0x04 and stores the cache list.
/// @param pCacheList: A pointer to store the cache list.
static void cpuid04hCommand(CacheList* pCacheList)
{
    pCacheList->cacheCount = 0;

    constexpr uint8_t maxCaches = sizeof(pCacheList->cacheInfo) / sizeof(pCacheList->cacheInfo[0]);

    while (pCacheList->cacheCount < maxCaches) {
        CacheInfo* pCacheInfo = &pCacheList->cacheInfo[pCacheList->cacheCount];

        unsigned int eax, ebx, ecx, edx;

        __asm__ __volatile__(
            "cpuid;" // cpuid komutunu çağır
            : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
            : "0"(0x04), "2"(pCacheList->cacheCount));

        if ((eax & 0x1f) == 0) {
            // printf("No more cache \n");
            break;
        }

        pCacheInfo->cacheType = eax & 0x1F;
        if (pCacheInfo->cacheType > 3) {
            pCacheInfo->cacheType = 0;
        }
        pCacheInfo->cacheLevel = (eax >> 5) & 0x7;
        pCacheInfo->selfInitializing = (eax >> 8) & 0x1;
        pCacheInfo->fullyAssociative = (eax >> 9) & 0x1;
        pCacheInfo->logicalCore = ((eax >> 14) & 0xFFF) + 1;
        pCacheInfo->physicalCore = ((eax >> 26) & 0x3F) + 1;
        pCacheInfo->coherencyLineSize = (ebx & 0xFFF) + 1;
        pCacheInfo->physicalLinePartitions = ((ebx >> 12) & 0x3FF) + 1;
        pCacheInfo->associativity = ((ebx >> 22) & 0x3FF) + 1;
        pCacheInfo->numberOfSets = ecx + 1;
        pCacheInfo->writeBackInvalidate = (edx >> 0) & 0x1;
        pCacheInfo->cacheInclusiveness = (edx >> 1) & 0x1;
        pCacheInfo->complexCacheIndexing = (edx >> 2) & 0x1;

        pCacheList->cacheCount++;
    }
}
