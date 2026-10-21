/**
 * @file condition_variable.hpp
 * @author Oguzhan KAYNAK
 * @brief Sync::ConditionVariable: wait until a condition, protected by a
 * Mutex, may have become true.
 *
 * Mesa semantics: notify only makes a waiter runnable. By the time it runs and
 * has the mutex again, the condition may be false again, so always wait in a
 * loop:
 *
 *     mutex.lock();
 *     while (!condition)
 *         cv.wait(mutex);
 *     ... use the state ...
 *     mutex.unlock();
 *
 * Threads only (the mutex needs an owner).
 */

#pragma once

#include <sync/mutex.hpp>
#include <sync/wait_queue.hpp>

namespace Sync {

class ConditionVariable {
public:
    ConditionVariable() = default;

    ConditionVariable(const ConditionVariable&) = delete;
    ConditionVariable& operator=(const ConditionVariable&) = delete;

    /// Unlock mutex and wait for a notify, then lock mutex again. The caller must own mutex.
    void wait(Mutex& mutex);

    /// Wake the longest waiting thread, if any.
    void notifyOne();

    /// Wake every waiting thread.
    void notifyAll();

    uint32_t waiterCount() const { return m_waiters.waiterCount(); }

private:
    WaitQueue m_waiters;
};

} // namespace Sync
