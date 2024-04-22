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
#include <kernel/cpu/cpu.hpp>
#include <kernel/cpu/cpuid.h>


static void printCPUFeatures(const CPUFeatures* features);
static void printProcessorType(const ProcessorType* processor);
static void printCacheInfo(const CacheList* cacheList);

static void cpuid_00h_command(uint32_t* p_maximumIndex, char* p_CPUstr);
static void cpuid_01h_command(ProcessorType* p_ProcessorType, CPUFeatures* p_CPUFeatures);
static void cpuid_04h_command(CacheList* p_CacheList);

void InitiliazeCPUInfo(CPU_Info* cpu_info)
{

    //cpuid_00h_command(&cpu_info->maksimumIndex, cpu_info->cpu);
	//cpuid_01h_command(&cpu_info->processorType, &cpu_info->CPUFeatures);
	cpuid_04h_command(&cpu_info->cacheList);

	if(cpu_info->CPUFeaturesInfo.HTT == 0 )
	{
		cpu_info->CPUTopologyInfo.logical_CPU_bits = 0;
		cpu_info->CPUTopologyInfo.core_bits = 0;
	}
}

void printCPUInfo(const CPU_Info* cpu_info)
{
	//printf("CPU: %s\n", cpu_info->cpu);
    //printf("Maksimum Index: %u\n", cpu_info->maksimumIndex);
	//printCPUFeatures(&cpu_info->CPUFeatures);
	//printf("\n");
	//printProcessorType(&cpu_info->processorType);
	//printCacheInfo(&cpu_info->cacheList);
}

/// @brief Prints the CPU features.
/// @param features: A pointer to CPU features to print.
static void printCPUFeatures(const CPUFeatures* features)
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

    printf("DS : %u\t", features->DS );
    printf("ACPI : %u\t", features->ACPI );
    printf("MMX : %u\t", features->MMX );
    printf("FXSR : %u\t", features->FXSR );
    printf("SSE : %u\t", features->SSE );
    printf("SSE2 : %u\t", features->SSE2 );
    printf("SS : %u\t", features->SS );
    printf("HTT : %u\t", features->HTT );
    printf("TM : %u\t", features->TM );
    printf("Reserved4 : %u\t", features->Reserved4 );
    printf("PBE : %u\t", features->PBE );
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
static void printCacheInfo(const CacheList* p_CacheList)
{
	const char* CacheTypeStrs[] = 
	{
		"Null",
		"D-Cache",
		"I-Cache",
		"Unified",
	};

	for(int i=0; i<p_CacheList->cacheCount; i++)
	{
		const CacheInfo* p_CacheInfo = &p_CacheList->cacheInfo[i];

		printf("Cache [%d]: \n", i);
		printf("cacheType: %u -> %s cacheLevel: %u self: %u \n",
			p_CacheInfo->cacheType, CacheTypeStrs[p_CacheInfo->cacheType], 
			p_CacheInfo->cacheLevel, p_CacheInfo->selfInitializing );

		printf("fully: %u LogicalCore: %u Cores: %u\n",
			p_CacheInfo->fullyAssociative, p_CacheInfo->LogicalCore, 
			p_CacheInfo->PhysicalCore);

		printf("LineSize: %u Partitions: %u associativity: %u\n",
			p_CacheInfo->coherencyLineSize, p_CacheInfo->physicalLinePartitions,
			p_CacheInfo->associativity);

		printf("numberOfSets: %u \n", p_CacheInfo->numberOfSets);

		printf("writeBackInvalidate: %u cacheInclusiveness: %u complexCacheIndexing:%u \n",
			p_CacheInfo->writeBackInvalidate,  p_CacheInfo->cacheInclusiveness, p_CacheInfo->complexCacheIndexing );

		printf("\n");

	}
}

/// @brief Executes the CPUID command with EAX=0x00 and stores the maximum index and CPU string.
/// @param p_maximumIndex: A pointer to store the maximum index.
/// @param p_CPUstr: A pointer to store the CPU string.
static void cpuid_00h_command(uint32_t* p_maximumIndex, char* p_CPUstr)
{
	unsigned int eax, ebx, ecx, edx;
	cpuid(0x00, &eax, &ebx, &ecx, &edx);
	
    sprintf(p_CPUstr  , "%c%c%c%c", ebx, (ebx >> 8), (ebx >> 16), (ebx >> 24));
	sprintf(p_CPUstr+4, "%c%c%c%c", edx, (edx >> 8), (edx >> 16), (edx >> 24));
	sprintf(p_CPUstr+8, "%c%c%c%c", ecx, (ecx >> 8), (ecx >> 16), (ecx >> 24));

	*p_maximumIndex = eax;
}

/// @brief Executes the CPUID command with EAX=0x01 and retrieves processor type and CPU features.
/// @param p_ProcessorType Pointer to a ProcessorType structure where the processor type information will be stored.
/// @param p_CPUFeatures Pointer to a CPUFeatures structure where the CPU feature information will be stored.
static void cpuid_01h_command(ProcessorType* p_ProcessorType, CPUFeatures* p_CPUFeatures)

{
	unsigned int eax, ebx, ecx, edx;
	cpuid(0x01, &eax, &ebx, &ecx, &edx);

    p_ProcessorType->stepping = eax & 0xF;
    p_ProcessorType->model = (eax >> 4) & 0xF;
    p_ProcessorType->family = (eax >> 8) & 0xF;
    p_ProcessorType->processorType = (eax >> 12) & 0x3;
    p_ProcessorType->reserved = (eax >> 14) & 0x3;
    p_ProcessorType->extendedModel = (eax >> 16) & 0xF;
    p_ProcessorType->extendedFamily = (eax >> 20) & 0xFF;

	p_CPUFeatures->SSE3                = (ecx & (1 << 0)) != 0;
	p_CPUFeatures->PCLMULQDQ           = (ecx & (1 << 1)) != 0;
	p_CPUFeatures->DTES64              = (ecx & (1 << 2)) != 0;
	p_CPUFeatures->MONITOR             = (ecx & (1 << 3)) != 0;
	p_CPUFeatures->DS_CPL              = (ecx & (1 << 4)) != 0;
	p_CPUFeatures->VMX                 = (ecx & (1 << 5)) != 0;
	p_CPUFeatures->SMX                 = (ecx & (1 << 6)) != 0;
	p_CPUFeatures->EIST                = (ecx & (1 << 7)) != 0;
	p_CPUFeatures->TM2                 = (ecx & (1 << 8)) != 0;
	p_CPUFeatures->SSSE3               = (ecx & (1 << 9)) != 0;
	p_CPUFeatures->CNXT_ID             = (ecx & (1 << 10)) != 0;
	p_CPUFeatures->SDBG                = (ecx & (1 << 11)) != 0;
	p_CPUFeatures->FMA                 = (ecx & (1 << 12)) != 0;
	p_CPUFeatures->CMPXCHG16B          = (ecx & (1 << 13)) != 0;
	p_CPUFeatures->xTPR_Update_Control = (ecx & (1 << 14)) != 0;
	p_CPUFeatures->PDCM                = (ecx & (1 << 15)) != 0;
	p_CPUFeatures->Reserved1           = (ecx & (1 << 16)) != 0;
	p_CPUFeatures->PCID                = (ecx & (1 << 17)) != 0;
	p_CPUFeatures->DCA                 = (ecx & (1 << 18)) != 0;
	p_CPUFeatures->SSE4_1              = (ecx & (1 << 19)) != 0;
	p_CPUFeatures->SSE4_2              = (ecx & (1 << 20)) != 0;
	p_CPUFeatures->x2APIC              = (ecx & (1 << 21)) != 0;
	p_CPUFeatures->MOVBE               = (ecx & (1 << 22)) != 0;
	p_CPUFeatures->POPCNT              = (ecx & (1 << 23)) != 0;
	p_CPUFeatures->TSC_Deadline        = (ecx & (1 << 24)) != 0;
	p_CPUFeatures->AESNI               = (ecx & (1 << 25)) != 0;
	p_CPUFeatures->XSAVE               = (ecx & (1 << 26)) != 0;
	p_CPUFeatures->OSXSAVE             = (ecx & (1 << 27)) != 0;
	p_CPUFeatures->AVX                 = (ecx & (1 << 28)) != 0;
	p_CPUFeatures->F16C                = (ecx & (1 << 29)) != 0;
	p_CPUFeatures->RDRAND              = (ecx & (1 << 30)) != 0;
	p_CPUFeatures->Not_Used            = (ecx & (1 << 31)) != 0;

	p_CPUFeatures->FPU  	 = (edx & (1 << 0)) != 0;
	p_CPUFeatures->VME  	 = (edx & (1 << 1)) != 0;
	p_CPUFeatures->DE 		 = (edx & (1 << 2)) != 0;
	p_CPUFeatures->PSE  	 = (edx & (1 << 3)) != 0;
	p_CPUFeatures->TSC  	 = (edx & (1 << 4)) != 0;
	p_CPUFeatures->MSR  	 = (edx & (1 << 5)) != 0;
	p_CPUFeatures->PAE  	 = (edx & (1 << 6)) != 0;
	p_CPUFeatures->MCE  	 = (edx & (1 << 7)) != 0;
	p_CPUFeatures->CX8  	 = (edx & (1 << 8)) != 0;
	p_CPUFeatures->APIC 	 = (edx & (1 << 9)) != 0;
	p_CPUFeatures->Reserved2 = (edx & (1 << 10)) != 0;
	p_CPUFeatures->SEP       = (edx & (1 << 11)) != 0;
	p_CPUFeatures->MTRR      = (edx & (1 << 12)) != 0;
	p_CPUFeatures->PGE       = (edx & (1 << 13)) != 0;
	p_CPUFeatures->MCA       = (edx & (1 << 14)) != 0;
	p_CPUFeatures->CMOV      = (edx & (1 << 15)) != 0;
	p_CPUFeatures->PAT       = (edx & (1 << 16)) != 0;
	p_CPUFeatures->PSE_36    = (edx & (1 << 17)) != 0;
	p_CPUFeatures->PSN       = (edx & (1 << 18)) != 0;
	p_CPUFeatures->CLFSH     = (edx & (1 << 19)) != 0;
	p_CPUFeatures->Reserved3 = (edx & (1 << 20)) != 0;
	p_CPUFeatures->DS  		 = (edx & (1 << 21)) != 0;
	p_CPUFeatures->ACPI  	 = (edx & (1 << 22)) != 0;
	p_CPUFeatures->MMX  	 = (edx & (1 << 23)) != 0;
	p_CPUFeatures->FXSR  	 = (edx & (1 << 24)) != 0;
	p_CPUFeatures->SSE  	 = (edx & (1 << 25)) != 0;
	p_CPUFeatures->SSE2  	 = (edx & (1 << 26)) != 0;
	p_CPUFeatures->SS  		 = (edx & (1 << 27)) != 0;
	p_CPUFeatures->HTT  	 = (edx & (1 << 28)) != 0;
	p_CPUFeatures->TM  		 = (edx & (1 << 29)) != 0;
	p_CPUFeatures->Reserved4 = (edx & (1 << 30)) != 0;
	p_CPUFeatures->PBE  	 = (edx & (1 << 31)) != 0;
}

/// @brief Executes the CPUID command with EAX=0x04 and stores the cache list.
/// @param p_CacheList: A pointer to store the cache list.
static void cpuid_04h_command(CacheList* p_CacheList)
{
	p_CacheList->cacheCount = 0;

	while(1)
	{
		CacheInfo* p_CacheInfo = &p_CacheList->cacheInfo[p_CacheList->cacheCount];

		unsigned int eax, ebx, ecx, edx;
		
		__asm__ __volatile__
		(
			"cpuid;"               // cpuid komutunu çağır
			: "=a" (eax), "=b" (ebx), "=c" (ecx), "=d" (edx)
			: "0" (0x04), "2" (p_CacheList->cacheCount)
		);

		if ((eax & 0x1f) == 0)
		{
			//printf("No more cache \n");
			break;
		}

		p_CacheInfo->cacheType = eax & 0x1F;
		if(p_CacheInfo->cacheType > 3)
		{
			p_CacheInfo->cacheType = 0;
		}
		p_CacheInfo->cacheLevel = (eax >> 5) & 0x7;
		p_CacheInfo->selfInitializing = (eax >> 8) & 0x1;
		p_CacheInfo->fullyAssociative = (eax >> 9) & 0x1;
		p_CacheInfo->LogicalCore = ((eax >> 14) & 0xFFF) +1;
		p_CacheInfo->PhysicalCore = ((eax >> 26) & 0x3F) +1;
		p_CacheInfo->coherencyLineSize = (ebx & 0xFFF) + 1;
		p_CacheInfo->physicalLinePartitions = ((ebx >> 12) & 0x3FF) + 1;
		p_CacheInfo->associativity = ((ebx >> 22) & 0x3FF) + 1;
		p_CacheInfo->numberOfSets = ecx + 1;
		p_CacheInfo->writeBackInvalidate = (edx >> 0) & 0x1;
		p_CacheInfo->cacheInclusiveness = (edx >> 1) & 0x1;
		p_CacheInfo->complexCacheIndexing = (edx >> 2) & 0x1;

		p_CacheList->cacheCount++;

	}
	

}
