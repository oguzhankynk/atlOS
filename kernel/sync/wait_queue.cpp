/**
 * @file wait_queue.cpp
 * @author Oguzhan KAYNAK
 * @brief Sync::WaitQueue: threads blocked until someone wakes them.
 */

#include <sync/wait_queue.hpp>

#include <core/panic.hpp>
#include <sched/thread.hpp>
#include <sync/interrupt_guard.hpp>

namespace Sync {

WaitQueue::WaitQueue(Threading::Scheduler& scheduler)
    : m_scheduler(scheduler)
{
}

WaitQueue::~WaitQueue()
{
    if (m_count)
        kernelPanic("WaitQueue: destroyed with waiting threads");
}

void WaitQueue::wait()
{
    InterruptGuard guard;

    if (m_count == capacity)
        kernelPanic("WaitQueue: full");

    m_waiters[(m_head + m_count) % capacity] = &m_scheduler.current();
    m_count++;

    // Interrupts stay off until another thread runs: its saved RFLAGS
    // decide then. They are off again when this thread continues here.
    m_scheduler.block();
}

Threading::TCB* WaitQueue::wakeOne()
{
    InterruptGuard guard;

    if (!m_count)
        return nullptr;

    Threading::TCB* tcb = m_waiters[m_head];
    m_waiters[m_head] = nullptr;
    m_head = (m_head + 1) % capacity;
    m_count--;

    m_scheduler.unblock(*tcb);
    return tcb;
}

void WaitQueue::wakeAll()
{
    InterruptGuard guard;

    while (wakeOne()) {
    }
}

} // namespace Sync
