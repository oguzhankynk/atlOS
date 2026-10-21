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

// Prints "name: " in a column, then the value
#define LABEL "  %-10s "

// Features as Linux names them in /proc/cpuinfo: edx of leaf 1, then ecx
struct FeatureName {
    const char* name;
    uint32_t present; // the bit from CPUID
};

// Appends name to the current line, starting a new one at about 76 columns
static void printFeature(const char* name, uint32_t& column)
{
    uint32_t length = strlen(name) + 1;
    if (column + length > 76) {
        printf("\n" LABEL, "");
        column = 13;
    }
    printf("%s ", name);
    column += length;
}

static void printCPUFeatures(const CPUFeatures* f)
{
    const FeatureName features[] = {
        { "fpu", f->fpu },
        { "vme", f->vme },
        { "de", f->de },
        { "pse", f->pse },
        { "tsc", f->tsc },
        { "msr", f->msr },
        { "pae", f->pae },
        { "mce", f->mce },
        { "cx8", f->cx8 },
        { "apic", f->apic },
        { "sep", f->sep },
        { "mtrr", f->mtrr },
        { "pge", f->pge },
        { "mca", f->mca },
        { "cmov", f->cmov },
        { "pat", f->pat },
        { "pse36", f->pse36 },
        { "pn", f->psn },
        { "clflush", f->clfsh },
        { "dts", f->ds },
        { "acpi", f->acpi },
        { "mmx", f->mmx },
        { "fxsr", f->fxsr },
        { "sse", f->sse },
        { "sse2", f->sse2 },
        { "ss", f->ss },
        { "ht", f->htt },
        { "tm", f->tm },
        { "pbe", f->pbe },
        { "sse3", f->sse3 },
        { "pclmulqdq", f->pclmulqdq },
        { "dtes64", f->dtes64 },
        { "monitor", f->monitor },
        { "ds_cpl", f->dsCpl },
        { "vmx", f->vmx },
        { "smx", f->smx },
        { "est", f->eist },
        { "tm2", f->tm2 },
        { "ssse3", f->ssse3 },
        { "cid", f->cnxtId },
        { "sdbg", f->sdbg },
        { "fma", f->fma },
        { "cx16", f->cmpxchg16b },
        { "xtpr", f->xTprUpdateControl },
        { "pdcm", f->pdcm },
        { "pcid", f->pcid },
        { "dca", f->dca },
        { "sse4_1", f->sse41 },
        { "sse4_2", f->sse42 },
        { "x2apic", f->x2APIC },
        { "movbe", f->movbe },
        { "popcnt", f->popcnt },
        { "tsc_deadline_timer", f->tscDeadline },
        { "aes", f->aesni },
        { "xsave", f->xsave },
        { "osxsave", f->osxsave },
        { "avx", f->avx },
        { "f16c", f->f16c },
        { "rdrand", f->rdrand },
        { "hypervisor", f->notUsed },
    };

    // Only what the CPU has
    printf(LABEL, "features");
    uint32_t column = 13;
    for (const FeatureName& feature : features) {
        if (feature.present)
            printFeature(feature.name, column);
    }
    printf("\n");
}

static void printProcessorType(const ProcessorType* p)
{
    // The extended fields count only for family 15 (family) and 6 or 15 (model)
    uint32_t family = p->family == 15 ? p->family + p->extendedFamily : p->family;
    uint32_t model = (p->family == 6 || p->family == 15) ? (p->extendedModel << 4) | p->model : p->model;
    printf(LABEL "%u, model %u, stepping %u\n", "family", family, model, p->stepping);
}

static void printCacheInfo(const CacheList* cacheList)
{
    static const char* const s_types[] = { "?", "d", "i", "" }; // data, instruction, unified

    printf(LABEL, "caches");
    if (!cacheList->cacheCount) {
        printf("not reported (CPUID leaf 4 is Intel's)\n");
        return;
    }
    for (int i = 0; i < cacheList->cacheCount; i++) {
        const CacheInfo& c = cacheList->cacheInfo[i];
        uint64_t size = (uint64_t)c.coherencyLineSize * c.physicalLinePartitions * c.associativity * c.numberOfSets;
        if (i)
            printf(LABEL, "");
        printf("L%u%-2s %6lu KB  %2u-way  %u byte lines%s\n", c.cacheLevel, s_types[c.cacheType & 3], size / 1024,
            c.associativity, c.coherencyLineSize, c.fullyAssociative ? ", fully associative" : "");
    }
}

void printCPUInfo(const CPUInfo* cpuInfo)
{
    printf(LABEL "%s\n", "vendor", cpuInfo->cpu);
    printProcessorType(&cpuInfo->processorType);
    printf(LABEL "0x%x\n", "max leaf", cpuInfo->maksimumIndex);
    printCPUFeatures(&cpuInfo->cpuFeaturesInfo);
    printCacheInfo(&cpuInfo->cacheList);
}

#undef LABEL

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
