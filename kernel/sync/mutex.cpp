/**
 * @file mutex.cpp
 * @author Oguzhan KAYNAK
 * @brief Sync::Mutex: mutual exclusion with an owner.
 */

#include <sync/mutex.hpp>

#include <core/panic.hpp>

namespace Sync {

// No InterruptGuard here: interrupt handlers never touch a mutex, and between
// threads nothing can happen in the middle without a yield.

Mutex::Mutex(Threading::Scheduler& scheduler)
    : m_waiters(scheduler)
{
}

void Mutex::lock()
{
    Threading::TCB* self = &m_waiters.scheduler().current();

    if (!m_owner) {
        m_owner = self;
        return;
    }

    if (m_owner == self)
        kernelPanic("Mutex: locked twice by the same thread");

    // unlock() makes us the owner before waking us
    m_waiters.wait();
}

bool Mutex::tryLock()
{
    if (m_owner)
        return false;
    m_owner = &m_waiters.scheduler().current();
    return true;
}

void Mutex::unlock()
{
    if (m_owner != &m_waiters.scheduler().current())
        kernelPanic("Mutex: unlocked by a thread that does not own it");

    // Hand over to the longest waiter (nullptr: nobody waits, unlocked)
    m_owner = m_waiters.wakeOne();
}

} // namespace Sync
