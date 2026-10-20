/**
 * @file wait_queue.hpp
 * @author Oguzhan KAYNAK
 * @brief Sync::WaitQueue: threads blocked until someone wakes them, first in
 * first out. The building block of Semaphore, Mutex and ConditionVariable.
 */

#pragma once

#include <stdint.h>

#include <sched/scheduler.hpp>

namespace Sync {

class WaitQueue {
public:
    /// As many waiters as the scheduler has tasks.
    static constexpr uint32_t capacity = Threading::Scheduler::maxTasks;

    explicit WaitQueue(Threading::Scheduler& scheduler);

    /// Panics if threads are still waiting: they would never wake up.
    ~WaitQueue();

    WaitQueue(const WaitQueue&) = delete;
    WaitQueue& operator=(const WaitQueue&) = delete;

    /**
     * @brief Block the running thread until wakeOne/wakeAll picks it.
     *
     * The caller checks its condition and calls wait() under one
     * InterruptGuard, so a wake up from an interrupt can not fall between
     * the check and the wait (a "lost wakeup"). Not for interrupt handlers.
     */
    void wait();

    /**
     * @brief Make the longest waiting thread runnable. Safe in an interrupt
     * handler. Does not switch threads; the woken one runs at the next yield.
     *
     * @return The woken thread, nullptr if nobody was waiting.
     */
    Threading::TCB* wakeOne();

    /// Wake every waiting thread. Safe in an interrupt handler.
    void wakeAll();

    bool isEmpty() const { return !m_count; }
    uint32_t waiterCount() const { return m_count; }

    Threading::Scheduler& scheduler() { return m_scheduler; }

private:
    Threading::Scheduler& m_scheduler;

    // Ring buffer: m_waiters[m_head] waits longest
    Threading::TCB* m_waiters[capacity] {};
    uint32_t m_head {};
    uint32_t m_count {};
};

} // namespace Sync
