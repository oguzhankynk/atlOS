/**
 * @file apic_timer.cpp
 * @author Oguzhan KAYNAK
 * @brief Drivers::ApicTimer: calibration against the PIT, periodic mode.
 */

#include <drivers/apic/apic_timer.hpp>

#include <stdio.h>

#include <arch/x86_64/idt/handlers.hpp>
#include <drivers/apic/apic.hpp>
#include <drivers/apic/local_apic.hpp>
#include <drivers/pit.hpp>
#include <sync/interrupt_guard.hpp>

namespace Drivers {

/// The divide configuration register's code for 16 (bits 3, 1..0: 0b0011)
static constexpr uint32_t divideBy16 = 0x3;

ApicTimer::ApicTimer(LocalApic& apic)
    : m_apic(apic)
{
}

bool ApicTimer::calibrate()
{
    uint32_t counted;
    bool waited;
    {
        // Nothing may come between starting the count and starting the PIT
        Sync::InterruptGuard guard;
        m_apic.write(LocalApic::RegisterTimerDivide, divideBy16);
        m_apic.write(LocalApic::RegisterLvtTimer, LocalApic::LvtMasked); // one shot, no interrupt
        m_apic.write(LocalApic::RegisterTimerInitialCount, 0xFFFFFFFF);
        waited = pitBusyWait(calibrationMicroseconds);
        counted = 0xFFFFFFFF - m_apic.read(LocalApic::RegisterTimerCurrentCount);
        m_apic.write(LocalApic::RegisterTimerInitialCount, 0); // stop
    }

    if (!waited || counted == 0) {
        printf("APIC timer: calibration failed (%s)\n", waited ? "the timer did not count" : "no PIT");
        return false;
    }
    m_frequency = (uint64_t)counted * (1000000 / calibrationMicroseconds);
    printf("APIC timer: %lu Hz (bus clock / %u)\n", m_frequency, divider);
    return true;
}

bool ApicTimer::start(uint32_t periodNs, TickHandler handler, void* arg)
{
    if (!m_frequency && !calibrate())
        return false;

    uint64_t count = m_frequency * periodNs / 1000000000ULL;
    if (count == 0 || count > 0xFFFFFFFF) {
        printf("APIC timer: period of %u ns is not supported\n", periodNs);
        return false;
    }

    Handlers::addInterruptHandler(ApicInterruptController::timerVector, handler, arg);

    // Periodic: at 0 the count reloads and the vector fires
    m_initialCount = static_cast<uint32_t>(count);
    m_apic.write(LocalApic::RegisterTimerDivide, divideBy16);
    m_apic.write(LocalApic::RegisterLvtTimer, LocalApic::LvtTimerPeriodic | ApicInterruptController::timerVector);
    m_apic.write(LocalApic::RegisterTimerInitialCount, m_initialCount);
    printf("APIC timer: period %u ns, count %u\n", periodNs, m_initialCount);
    return true;
}

} // namespace Drivers
