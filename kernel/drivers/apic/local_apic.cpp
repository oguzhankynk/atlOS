/**
 * @file local_apic.cpp
 * @author Oguzhan KAYNAK
 * @brief Drivers::LocalApic: enabling the xAPIC.
 */

#include <drivers/apic/local_apic.hpp>

#include <arch/x86_64/cpu/cpuid.hpp>
#include <arch/x86_64/cpu/msr.hpp>

namespace Drivers {

static constexpr uint32_t cpuidApic = 1 << 9;
static constexpr uint64_t apicBaseEnable = 1 << 11;
static constexpr uint32_t spuriousEnable = 1 << 8;

bool LocalApic::isPresent()
{
    unsigned int eax, ebx, ecx, edx;
    cpuid(1, &eax, &ebx, &ecx, &edx);
    return edx & cpuidApic;
}

LocalApic::LocalApic(volatile void* registers, uint64_t physAddress)
    : m_registers(static_cast<volatile uint32_t*>(registers))
    , m_physAddress(physAddress)
{
}

void LocalApic::enable(uint8_t spuriousVector)
{
    // The global enable: firmware leaves it on, but it may have been cleared
    uint64_t base;
    if (readMsr(msrApicBase, &base) && !(base & apicBaseEnable))
        writeMsr(msrApicBase, base | apicBaseEnable);

    // Priority 0: nothing is held back
    write(RegisterTaskPriority, 0);

    // The software enable, and where an interrupt that went away before it was taken goes
    write(RegisterSpurious, spuriousEnable | spuriousVector);
}

} // namespace Drivers
