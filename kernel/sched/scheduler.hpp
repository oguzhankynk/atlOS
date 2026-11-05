/**
 * @file scheduler.hpp
 * @author Oguzhan KAYNAK ()
 * @brief Cooperative round robin scheduler.
 *
 * Threads only change on yield/sleep/block/exit; the timer interrupt just
 * counts ticks (and charges each one to the running task) and never switches
 * threads. Every scheduler has an idle thread that runs when no other task
 * can. There is no global scheduler: a thread reaches its scheduler through
 * the running task (currentScheduler()).
 *
 * Copyright © 2024 Oguzhan KAYNAK
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the “Software”), to deal in the Software
 * without  restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit
 * persons to whom the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
 * PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#pragma once

#include <sched/thread.hpp>
#include <stdint.h>
#include <sync/atomic.hpp>

namespace Drivers {
class TickSource;
}

namespace Memory {
class AddressSpace;
}

namespace Threading {

class Scheduler {
public:
    static constexpr uint32_t maxTasks = 16;
    static constexpr uint32_t tickPeriodNs = 1000000; // 1 ms

    /// Queue index of the idle thread: after task 0, which is never removed either
    static constexpr uint32_t idleIndex = 1;

    /**
     * @brief The calling flow becomes task 0 (it has no Thread object), the
     * idle thread task 1 (its stack comes from space), and the 1 ms tick is
     * started on ticks. Panics if ticks can not do 1 ms.
     */
    Scheduler(Drivers::TickSource& ticks, Memory::AddressSpace& space);

    /// The constructing flow stops being this scheduler's task 0: the task it was before is current again.
    ~Scheduler();

    Scheduler(const Scheduler&) = delete;
    Scheduler& operator=(const Scheduler&) = delete;

    void addTask(Thread& thread);

    /**
     * @brief Remove a thread from the run queue. No-op if it is not queued
     * (e.g. it has already exited). The running thread can not remove itself.
     */
    void removeTask(Thread& thread);

    /// Run the next runnable thread, if there is one.
    void yield();

    /// Let other threads run for at least ms milliseconds (ticks).
    void sleep(uint64_t ms);

    /// Stop running the current thread until someone calls unblock on it.
    void block();

    /// Make a Blocked thread runnable again. Other states are left alone.
    void unblock(TCB& tcb);

    /**
     * @brief End the current thread. It leaves the run queue and never runs again;
     * its owner still has to delete the Thread object.
     */
    [[noreturn]] void exit();

    /// TCB of the running thread.
    TCB& current();

    /// Milliseconds (ticks) since construction.
    uint64_t ticks() const { return m_ticks.load(); }

    /// Number of queued tasks, including the flow that constructed the scheduler and the idle thread.
    uint32_t taskCount() const { return m_threadCount; }

    /// Context switches since construction.
    uint64_t switchCount() const { return m_switchCount; }

    /// TCB of queued task index (0 .. taskCount() - 1), for listings.
    const TCB& task(uint32_t index) const { return *m_readyQueue[index]; }

    /// Index of the running task in the queue.
    uint32_t currentIndex() const { return m_currentThreadIndex; }

private:
    static void onTick(void* scheduler);

    static void idleLoop();

    void wakeSleepers();
    void removeAt(uint32_t index);
    void switchTo(uint32_t nextIndex);
    void schedule();

    // Context of the constructing flow (kernelMain). It has no Thread object,
    // but it must own a TCB so that the first switchTask has somewhere to save it.
    TCB m_bootTcb {};

    TCB* m_readyQueue[maxTasks] {};
    uint32_t m_threadCount {};
    uint32_t m_currentThreadIndex {};

    // Runs when no other task can, see idleLoop
    Thread* m_idleThread {};

    // Incremented by the timer interrupt, read by threads
    Sync::Atomic<uint64_t> m_ticks;

    // The task the timer interrupt charges its tick to. Set together with the
    // queue index, but one aligned 8 byte store: the interrupt never sees it
    // half changed. volatile: the interrupt reads it behind the compiler's back
    TCB* volatile m_running {};

    uint64_t m_switchCount {};

    // What currentTask() was before this scheduler took the constructing flow over
    TCB* m_previousTask {};
};

/**
 * @brief The task running now. Every switch of every scheduler updates it, so
 * code can find "this thread" and its scheduler without being handed one:
 * the sync objects (WaitQueue, Mutex, ...), Thread(entry), the system calls.
 *
 * A boundary global like Core::setConsole (one CPU runs one task at a time);
 * with several CPUs it becomes one per CPU. See kernel/sched/README.md.
 */
TCB& currentTask();

/// The scheduler of the running task.
Scheduler& currentScheduler();

// The running task's scheduler, for threads
void yield();
void sleep(uint64_t ms);
[[noreturn]] void exitThread();

} // namespace Threading
