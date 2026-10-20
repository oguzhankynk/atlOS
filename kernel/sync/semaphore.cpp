/**
 * @file semaphore.cpp
 * @author Oguzhan KAYNAK
 * @brief Sync::Semaphore: a counting semaphore.
 */

#include <sync/semaphore.hpp>

#include <core/panic.hpp>
#include <sync/interrupt_guard.hpp>

namespace Sync {

Semaphore::Semaphore(Threading::Scheduler& scheduler, uint32_t initial, uint32_t max)
    : m_waiters(scheduler)
    , m_count(initial)
    , m_max(max)
{
    if (!max || initial > max)
        kernelPanic("Semaphore: needs 0 < max and initial <= max");
}

void Semaphore::take()
{
    // give() may run in an interrupt handler: check and wait as one step
    InterruptGuard guard;

    if (m_count) {
        m_count--;
        return;
    }

    // give() hands its unit to us directly, the count stays 0: nobody can
    // take it between the wake up and our return
    m_waiters.wait();
}

bool Semaphore::tryTake()
{
    InterruptGuard guard;

    if (!m_count)
        return false;
    m_count--;
    return true;
}

bool Semaphore::give()
{
    InterruptGuard guard;

    if (m_waiters.wakeOne())
        return true;

    if (m_count == m_max)
        return false;
    m_count++;
    return true;
}

} // namespace Sync
