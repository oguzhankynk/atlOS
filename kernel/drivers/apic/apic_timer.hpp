/**
 * @file apic_timer.hpp
 * @author Oguzhan KAYNAK
 * @brief The local APIC's timer as a periodic tick, its frequency measured
 * against the PIT.
 */

#pragma once

#include <stdint.h>

#include <drivers/tick_source.hpp>

namespace Drivers {

class LocalApic;

class ApicTimer final : public TickSource {
public:
    /// The timer counts the bus clock divided by this
    static constexpr uint32_t divider = 16;

    /// How long calibrate measures
    static constexpr uint32_t calibrationMicroseconds = 10000;

    explicit ApicTimer(LocalApic& apic);

    ApicTimer(const ApicTimer&) = delete;
    ApicTimer& operator=(const ApicTimer&) = delete;

    /**
     * @brief Count down from the largest value while the PIT waits 10 ms:
     * what was counted, times 100, is the frequency. Interrupts are off meanwhile.
     * @return false if the PIT did not finish or the timer did not count.
     */
    bool calibrate();

    /**
     * @brief Calibrate (once), then count periodNs in periodic mode and call
     * handler(arg) on ApicInterruptController::timerVector each time.
     */
    bool start(uint32_t periodNs, TickHandler handler, void* arg) override;

    /// Counts per second (after the divider); 0 before calibrate
    uint64_t frequency() const { return m_frequency; }

    /// The count of one period; 0 before start
    uint32_t initialCount() const { return m_initialCount; }

private:
    LocalApic& m_apic;
    uint64_t m_frequency {};
    uint32_t m_initialCount {};
};

} // namespace Drivers
