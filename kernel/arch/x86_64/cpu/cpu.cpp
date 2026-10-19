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
#include <stdio.h>
#include <string.h>
#include <arch/x86_64/cpu/cpu.hpp>
#include <arch/x86_64/cpu/cpuid.hpp>

static void printCPUFeatures(const CPUFeatures *features);
static void printProcessorType(const ProcessorType *processor);
static void printCacheInfo(const CacheList *cacheList);

static void cpuid00hCommand(uint32_t *pMaximumIndex, char *cpuString);
static void cpuid01hCommand(ProcessorType *pProcessorType, CPUFeatures *pCpuFeatures);
static void cpuid04hCommand(CacheList *pCacheList);

void initializeCpuInfo(CPUInfo *cpuInfo)
{
	// Fields a leaf does not fill (e.g. the topology) stay zero
	memset(cpuInfo, 0, sizeof(*cpuInfo));

	cpuid00hCommand(&cpuInfo->maksimumIndex, cpuInfo->cpu);

	if (cpuInfo->maksimumIndex >= 0x01)
		cpuid01hCommand(&cpuInfo->processorType, &cpuInfo->CPUFeaturesInfo);

	// Above the highest supported leaf CPUID returns unrelated data
	if (cpuInfo->maksimumIndex >= 0x04)
		cpuid04hCommand(&cpuInfo->cacheList);
}

void printCPUInfo(const CPUInfo *cpuInfo)
{
	printf("CPU: %s\n", cpuInfo->cpu);
	printf("Maksimum Index: %u\n", cpuInfo->maksimumIndex);
	printCPUFeatures(&cpuInfo->CPUFeaturesInfo);
	printf("\n");
	printProcessorType(&cpuInfo->processorType);
	printCacheInfo(&cpuInfo->cacheList);
}

/// @brief Prints the CPU features.
/// @param features: A pointer to CPU features to print.
static void printCPUFeatures(const CPUFeatures *features)
{
	printf("SSE3: %u\t", features->SSE3);
	printf("PCLMULQDQ: %u\t", features->PCLMULQDQ);
	printf("DTES64: %u\t", features->DTES64);
	printf("MONITOR: %u\t", features->MONITOR);
	printf("DS_CPL: %u\t", features->DS_CPL);
	printf("VMX: %u\t", features->VMX);
	printf("SMX: %u\t", features->SMX);
	printf("EIST: %u\t", features->EIST);
	printf("TM2: %u\t", features->TM2);
	printf("SSSE3: %u\t", features->SSSE3);
	printf("CNXT_ID: %u\t", features->CNXT_ID);
	printf("SDBG: %u\t", features->SDBG);
	printf("FMA: %u\t", features->FMA);
	printf("CMPXCHG16B: %u\t", features->CMPXCHG16B);
	printf("xTPR_Update_Control: %u\t", features->xTPR_Update_Control);
	printf("PDCM: %u\t", features->PDCM);
	printf("Reserved1: %u\t", features->Reserved1);
	printf("PCID: %u\t", features->PCID);
	printf("DCA: %u\t", features->DCA);
	printf("SSE4_1: %u\t", features->SSE4_1);
	printf("SSE4_2: %u\t", features->SSE4_2);
	printf("x2APIC: %u\t", features->x2APIC);
	printf("MOVBE: %u\t", features->MOVBE);
	printf("POPCNT: %u\t", features->POPCNT);
	printf("TSC_Deadline: %u\t", features->TSC_Deadline);
	printf("AESNI: %u\t", features->AESNI);
	printf("XSAVE: %u\t", features->XSAVE);
	printf("OSXSAVE: %u\t", features->OSXSAVE);
	printf("AVX: %u\t", features->AVX);
	printf("F16C: %u\t", features->F16C);
	printf("RDRAND: %u\t", features->RDRAND);
	printf("Not_Used: %u\t", features->Not_Used);
	printf("FPU: %u\t", features->FPU);
	printf("VME: %u\t", features->VME);
	printf("DE: %u\t", features->DE);
	printf("PSE: %u\t", features->PSE);
	printf("TSC: %u\t", features->TSC);
	printf("MSR: %u\t", features->MSR);
	printf("PAE: %u\t", features->PAE);
	printf("MCE: %u\t", features->MCE);
	printf("CX8: %u\t", features->CX8);
	printf("APIC: %u\t", features->APIC);
	printf("Reserved2: %u\t", features->Reserved2);
	printf("SEP: %u\t", features->SEP);
	printf("MTRR: %u\t", features->MTRR);
	printf("PGE: %u\t", features->PGE);
	printf("MCA: %u\t", features->MCA);
	printf("CMOV: %u\t", features->CMOV);
	printf("PAT: %u\t", features->PAT);
	printf("PSE_36: %u\t", features->PSE_36);
	printf("PSN: %u\t", features->PSN);
	printf("CLFSH: %u\t", features->CLFSH);
	printf("Reserved3: %u\t", features->Reserved3);

	printf("DS : %u\t", features->DS);
	printf("ACPI : %u\t", features->ACPI);
	printf("MMX : %u\t", features->MMX);
	printf("FXSR : %u\t", features->FXSR);
	printf("SSE : %u\t", features->SSE);
	printf("SSE2 : %u\t", features->SSE2);
	printf("SS : %u\t", features->SS);
	printf("HTT : %u\t", features->HTT);
	printf("TM : %u\t", features->TM);
	printf("Reserved4 : %u\t", features->Reserved4);
	printf("PBE : %u\t", features->PBE);
}

/// @brief Prints the processor type.
/// @param processor: A pointer to the processor type information to print.
static void printProcessorType(const ProcessorType *processor)
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
static void printCacheInfo(const CacheList *pCacheList)
{
	const char *cacheTypeStrs[] =
		{
			"Null",
			"D-Cache",
			"I-Cache",
			"Unified",
		};

	for (int i = 0; i < pCacheList->cacheCount; i++)
	{
		const CacheInfo *pCacheInfo = &pCacheList->cacheInfo[i];

		printf("Cache [%d]: \n", i);
		printf("cacheType: %u -> %s cacheLevel: %u self: %u \n",
			   pCacheInfo->cacheType, cacheTypeStrs[pCacheInfo->cacheType],
			   pCacheInfo->cacheLevel, pCacheInfo->selfInitializing);

		printf("fully: %u LogicalCore: %u Cores: %u\n",
			   pCacheInfo->fullyAssociative, pCacheInfo->LogicalCore,
			   pCacheInfo->PhysicalCore);

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
static void cpuid00hCommand(uint32_t *pMaximumIndex, char *cpuString)
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
static void cpuid01hCommand(ProcessorType *pProcessorType, CPUFeatures *pCpuFeatures)

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

	pCpuFeatures->SSE3 = (ecx & (1 << 0)) != 0;
	pCpuFeatures->PCLMULQDQ = (ecx & (1 << 1)) != 0;
	pCpuFeatures->DTES64 = (ecx & (1 << 2)) != 0;
	pCpuFeatures->MONITOR = (ecx & (1 << 3)) != 0;
	pCpuFeatures->DS_CPL = (ecx & (1 << 4)) != 0;
	pCpuFeatures->VMX = (ecx & (1 << 5)) != 0;
	pCpuFeatures->SMX = (ecx & (1 << 6)) != 0;
	pCpuFeatures->EIST = (ecx & (1 << 7)) != 0;
	pCpuFeatures->TM2 = (ecx & (1 << 8)) != 0;
	pCpuFeatures->SSSE3 = (ecx & (1 << 9)) != 0;
	pCpuFeatures->CNXT_ID = (ecx & (1 << 10)) != 0;
	pCpuFeatures->SDBG = (ecx & (1 << 11)) != 0;
	pCpuFeatures->FMA = (ecx & (1 << 12)) != 0;
	pCpuFeatures->CMPXCHG16B = (ecx & (1 << 13)) != 0;
	pCpuFeatures->xTPR_Update_Control = (ecx & (1 << 14)) != 0;
	pCpuFeatures->PDCM = (ecx & (1 << 15)) != 0;
	pCpuFeatures->Reserved1 = (ecx & (1 << 16)) != 0;
	pCpuFeatures->PCID = (ecx & (1 << 17)) != 0;
	pCpuFeatures->DCA = (ecx & (1 << 18)) != 0;
	pCpuFeatures->SSE4_1 = (ecx & (1 << 19)) != 0;
	pCpuFeatures->SSE4_2 = (ecx & (1 << 20)) != 0;
	pCpuFeatures->x2APIC = (ecx & (1 << 21)) != 0;
	pCpuFeatures->MOVBE = (ecx & (1 << 22)) != 0;
	pCpuFeatures->POPCNT = (ecx & (1 << 23)) != 0;
	pCpuFeatures->TSC_Deadline = (ecx & (1 << 24)) != 0;
	pCpuFeatures->AESNI = (ecx & (1 << 25)) != 0;
	pCpuFeatures->XSAVE = (ecx & (1 << 26)) != 0;
	pCpuFeatures->OSXSAVE = (ecx & (1 << 27)) != 0;
	pCpuFeatures->AVX = (ecx & (1 << 28)) != 0;
	pCpuFeatures->F16C = (ecx & (1 << 29)) != 0;
	pCpuFeatures->RDRAND = (ecx & (1 << 30)) != 0;
	pCpuFeatures->Not_Used = (ecx & (1 << 31)) != 0;

	pCpuFeatures->FPU = (edx & (1 << 0)) != 0;
	pCpuFeatures->VME = (edx & (1 << 1)) != 0;
	pCpuFeatures->DE = (edx & (1 << 2)) != 0;
	pCpuFeatures->PSE = (edx & (1 << 3)) != 0;
	pCpuFeatures->TSC = (edx & (1 << 4)) != 0;
	pCpuFeatures->MSR = (edx & (1 << 5)) != 0;
	pCpuFeatures->PAE = (edx & (1 << 6)) != 0;
	pCpuFeatures->MCE = (edx & (1 << 7)) != 0;
	pCpuFeatures->CX8 = (edx & (1 << 8)) != 0;
	pCpuFeatures->APIC = (edx & (1 << 9)) != 0;
	pCpuFeatures->Reserved2 = (edx & (1 << 10)) != 0;
	pCpuFeatures->SEP = (edx & (1 << 11)) != 0;
	pCpuFeatures->MTRR = (edx & (1 << 12)) != 0;
	pCpuFeatures->PGE = (edx & (1 << 13)) != 0;
	pCpuFeatures->MCA = (edx & (1 << 14)) != 0;
	pCpuFeatures->CMOV = (edx & (1 << 15)) != 0;
	pCpuFeatures->PAT = (edx & (1 << 16)) != 0;
	pCpuFeatures->PSE_36 = (edx & (1 << 17)) != 0;
	pCpuFeatures->PSN = (edx & (1 << 18)) != 0;
	pCpuFeatures->CLFSH = (edx & (1 << 19)) != 0;
	pCpuFeatures->Reserved3 = (edx & (1 << 20)) != 0;
	pCpuFeatures->DS = (edx & (1 << 21)) != 0;
	pCpuFeatures->ACPI = (edx & (1 << 22)) != 0;
	pCpuFeatures->MMX = (edx & (1 << 23)) != 0;
	pCpuFeatures->FXSR = (edx & (1 << 24)) != 0;
	pCpuFeatures->SSE = (edx & (1 << 25)) != 0;
	pCpuFeatures->SSE2 = (edx & (1 << 26)) != 0;
	pCpuFeatures->SS = (edx & (1 << 27)) != 0;
	pCpuFeatures->HTT = (edx & (1 << 28)) != 0;
	pCpuFeatures->TM = (edx & (1 << 29)) != 0;
	pCpuFeatures->Reserved4 = (edx & (1 << 30)) != 0;
	pCpuFeatures->PBE = (edx & (1 << 31)) != 0;
}

/// @brief Executes the CPUID command with EAX=0x04 and stores the cache list.
/// @param pCacheList: A pointer to store the cache list.
static void cpuid04hCommand(CacheList *pCacheList)
{
	pCacheList->cacheCount = 0;

	constexpr uint8_t maxCaches = sizeof(pCacheList->cacheInfo) / sizeof(pCacheList->cacheInfo[0]);

	while (pCacheList->cacheCount < maxCaches)
	{
		CacheInfo *pCacheInfo = &pCacheList->cacheInfo[pCacheList->cacheCount];

		unsigned int eax, ebx, ecx, edx;

		__asm__ __volatile__(
			"cpuid;" // cpuid komutunu çağır
			: "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
			: "0"(0x04), "2"(pCacheList->cacheCount));

		if ((eax & 0x1f) == 0)
		{
			// printf("No more cache \n");
			break;
		}

		pCacheInfo->cacheType = eax & 0x1F;
		if (pCacheInfo->cacheType > 3)
		{
			pCacheInfo->cacheType = 0;
		}
		pCacheInfo->cacheLevel = (eax >> 5) & 0x7;
		pCacheInfo->selfInitializing = (eax >> 8) & 0x1;
		pCacheInfo->fullyAssociative = (eax >> 9) & 0x1;
		pCacheInfo->LogicalCore = ((eax >> 14) & 0xFFF) + 1;
		pCacheInfo->PhysicalCore = ((eax >> 26) & 0x3F) + 1;
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
