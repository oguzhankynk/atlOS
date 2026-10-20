/**
 * @file mutex.hpp
 * @author Oguzhan KAYNAK
 * @brief Sync::Mutex: mutual exclusion with an owner. Waiting threads sleep.
 *
 * With a cooperative scheduler a thread only loses the CPU where it yields,
 * sleeps or waits, so code that does none of those inside its critical
 * section needs no mutex. A mutex is for sections that wait in the middle
 * (e.g. for a disk interrupt while a file system structure is half updated).
 * Threads only: an interrupt handler has no thread of its own to be the owner.
 */

#pragma once

#include <sync/wait_queue.hpp>

namespace Sync {

class Mutex {
public:
    explicit Mutex(Threading::Scheduler& scheduler);

    Mutex(const Mutex&) = delete;
    Mutex& operator=(const Mutex&) = delete;

    /// Lock, waiting while another thread owns it. Panics if the caller already owns it.
    void lock();

    /// Lock if nobody owns it, never wait.
    bool tryLock();

    /// Unlock; the longest waiting thread becomes the owner. Panics if the caller is not the owner.
    void unlock();

    bool isLocked() const { return m_owner; }
    bool isOwnedByCurrent() { return m_owner == &m_waiters.scheduler().current(); }

private:
    WaitQueue m_waiters;
    Threading::TCB* m_owner {};
};

} // namespace Sync
