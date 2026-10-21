/**
 * @file semaphore.hpp
 * @author Oguzhan KAYNAK
 * @brief Sync::Semaphore: a counting semaphore. take() waits for a unit,
 * give() returns one, also from an interrupt handler.
 */

#pragma once

#include <stdint.h>

#include <sync/wait_queue.hpp>

namespace Sync {

class Semaphore {
public:
    /**
     * @brief A semaphore with initial units out of at most max.
     * max = 1 is a binary semaphore. Panics if max is 0 or initial > max.
     */
    Semaphore(uint32_t initial, uint32_t max);

    Semaphore(const Semaphore&) = delete;
    Semaphore& operator=(const Semaphore&) = delete;

    /// Take a unit, wait while there is none. Not for interrupt handlers.
    void take();

    /// Take a unit if there is one, never wait.
    bool tryTake();

    /**
     * @brief Give a unit back: straight to the longest waiting thread if there
     * is one, else to the count. Safe in an interrupt handler.
     *
     * @return false if the count is already at max (the unit is dropped).
     */
    bool give();

    uint32_t count() const { return m_count; }
    uint32_t waiterCount() const { return m_waiters.waiterCount(); }

private:
    WaitQueue m_waiters;
    uint32_t m_count;
    uint32_t m_max;
};

} // namespace Sync
