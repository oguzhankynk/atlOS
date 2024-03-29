#ifndef A046BEC9_DED9_4255_85EE_5065946448E0
#define A046BEC9_DED9_4255_85EE_5065946448E0

#include <stdint.h>

#if defined(__cplusplus)
extern "C" {
#endif

struct ProcessorType
{
    uint32_t stepping : 4;       // Stepping (4 bits)
    uint32_t model : 4;          // Model (4 bits)
    uint32_t family : 4;         // Family (4 bits)
    uint32_t processorType : 2;  // Processor Type (2 bits)
    uint32_t reserved : 2;       // Reserved (2 bits)
    uint32_t extendedModel : 4;  // Extended Model (4 bits)
    uint32_t extendedFamily : 8; // Extended Family (8 bits)
};

struct CPUFeatures
{
    uint32_t SSE3 : 1;
    uint32_t PCLMULQDQ : 1;
    uint32_t DTES64 : 1;
    uint32_t MONITOR : 1;
    uint32_t DS_CPL : 1;
    uint32_t VMX : 1;
    uint32_t SMX : 1;
    uint32_t EIST : 1;
    uint32_t TM2 : 1;
    uint32_t SSSE3 : 1;
    uint32_t CNXT_ID : 1;
    uint32_t SDBG : 1;
    uint32_t FMA : 1;
    uint32_t CMPXCHG16B : 1;
    uint32_t xTPR_Update_Control : 1;
    uint32_t PDCM : 1;
    uint32_t Reserved1 : 1;
    uint32_t PCID : 1;
    uint32_t DCA : 1;
    uint32_t SSE4_1 : 1;
    uint32_t SSE4_2 : 1;
    uint32_t x2APIC : 1;
    uint32_t MOVBE : 1;
    uint32_t POPCNT : 1;
    uint32_t TSC_Deadline : 1;
    uint32_t AESNI : 1;
    uint32_t XSAVE : 1;
    uint32_t OSXSAVE : 1;
    uint32_t AVX : 1;
    uint32_t F16C : 1;
    uint32_t RDRAND : 1;
    uint32_t Not_Used : 1;
    uint32_t FPU : 1;
    uint32_t VME : 1;
    uint32_t DE : 1;
    uint32_t PSE : 1;
    uint32_t TSC : 1;
    uint32_t MSR : 1;
    uint32_t PAE : 1;
    uint32_t MCE : 1;
    uint32_t CX8 : 1;
    uint32_t APIC : 1;
    uint32_t Reserved2 : 1;
    uint32_t SEP : 1;
    uint32_t MTRR : 1;
    uint32_t PGE : 1;
    uint32_t MCA : 1;
    uint32_t CMOV : 1;
    uint32_t PAT : 1;
    uint32_t PSE_36 : 1;
    uint32_t PSN : 1;
    uint32_t CLFSH : 1;
    uint32_t Reserved3 : 1;
    uint32_t DS : 1;
    uint32_t ACPI : 1;
    uint32_t MMX : 1;
    uint32_t FXSR : 1;
    uint32_t SSE : 1;
    uint32_t SSE2 : 1;
    uint32_t SS : 1;
    uint32_t HTT : 1;
    uint32_t TM : 1;
    uint32_t Reserved4 : 1;
    uint32_t PBE : 1;
};

struct CPUTopology
{
    uint32_t logical_CPU_bits;
    uint32_t core_bits;
} ;

struct CacheInfo
{
    uint32_t cacheType : 5;
    uint32_t cacheLevel : 3;
    uint32_t selfInitializing : 1;
    uint32_t fullyAssociative : 1;
    uint32_t LogicalCore : 12;
    uint32_t PhysicalCore : 6;
    uint32_t coherencyLineSize : 12;
    uint32_t physicalLinePartitions : 10;
    uint32_t associativity : 10;
    uint32_t numberOfSets : 32;
    uint32_t writeBackInvalidate : 1;
    uint32_t cacheInclusiveness : 1;
    uint32_t complexCacheIndexing : 1;
};

struct CacheList
{
    uint8_t cacheCount;
    CacheInfo cacheInfo[8];
};

struct CPU_Info
{
    char cpu[32];
    uint32_t maksimumIndex;
    CPUFeatures CPUFeaturesInfo;
    ProcessorType processorType;
    CPUTopology CPUTopologyInfo;
    CacheList cacheList;
} ;

void InitiliazeCPUInfo(CPU_Info *cpu_info);
void printCPUInfo(const CPU_Info *cpu_info);

#if defined(__cplusplus)
} /* extern "C" */
#endif

#endif /* A046BEC9_DED9_4255_85EE_5065946448E0 */
