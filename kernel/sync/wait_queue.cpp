/**
 * @file wait_queue.cpp
 * @author Oguzhan KAYNAK
 * @brief Sync::WaitQueue: threads blocked until someone wakes them.
 */

#include <sync/wait_queue.hpp>

#include <core/panic.hpp>
#include <sched/scheduler.hpp>
#include <sched/thread.hpp>
#include <sync/interrupt_guard.hpp>

namespace Sync {

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

    Threading::TCB& self = Threading::currentTask();
    m_waiters[(m_head + m_count) % capacity] = &self;
    m_count++;

    // Interrupts stay off until another thread runs: its saved RFLAGS
    // decide then. They are off again when this thread continues here.
    self.scheduler->block();
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

    tcb->scheduler->unblock(*tcb);
    return tcb;
}

void WaitQueue::wakeAll()
{
    InterruptGuard guard;

    while (wakeOne()) {
    }
}

} // namespace Sync
