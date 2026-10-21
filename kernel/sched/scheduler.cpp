#include <arch/x86_64/gdt/gdt.hpp>
#include <arch/x86_64/io/ioport.hpp>
#include <core/panic.hpp>
#include <drivers/tick_source.hpp>
#include <sched/scheduler.hpp>
#include <sched/thread.hpp>
#include <stdio.h>

extern "C" void switchTask(Threading::Registers* from, Threading::Registers* to);

namespace Threading {

// The task the CPU runs now, see currentTask()
static TCB* s_currentTask {};

TCB& currentTask()
{
    if (!s_currentTask)
        kernelPanic("currentTask before the first Scheduler");
    return *s_currentTask;
}

Scheduler& currentScheduler()
{
    return *currentTask().scheduler;
}

void yield()
{
    currentScheduler().yield();
}

void sleep(uint64_t ms)
{
    currentScheduler().sleep(ms);
}

void exitThread()
{
    currentScheduler().exit();
}

static bool isRunnable(const TCB* tcb)
{
    return tcb->state == ThreadState::Ready || tcb->state == ThreadState::Running;
}

Scheduler::Scheduler(Drivers::TickSource& ticks)
{
    m_bootTcb.state = ThreadState::Running;
    m_bootTcb.name = "kernel"; // the flow that constructed the scheduler (kernelMain)
    m_bootTcb.scheduler = this;

    // The constructing flow now runs as this scheduler's task 0
    m_previousTask = s_currentTask;
    s_currentTask = &m_bootTcb;
    m_readyQueue[0] = &m_bootTcb;
    m_threadCount = 1;
    m_currentThreadIndex = 0;

    if (!ticks.start(tickPeriodNs, onTick, this))
        kernelPanic("Scheduler: the tick source does not support a 1 ms period");
}

Scheduler::~Scheduler()
{
    if (s_currentTask == &m_bootTcb)
        s_currentTask = m_previousTask;
}

void Scheduler::onTick(void* scheduler)
{
    auto* self = static_cast<Scheduler*>(scheduler);
    self->m_ticks.fetchAdd(1);
}

void Scheduler::wakeSleepers()
{
    uint64_t now = m_ticks.load();
    for (uint32_t i = 0; i < m_threadCount; i++) {
        if (m_readyQueue[i]->state == ThreadState::Sleeping && now >= m_readyQueue[i]->wakeTick)
            m_readyQueue[i]->state = ThreadState::Ready;
    }
}

void Scheduler::removeAt(uint32_t index)
{
    for (uint32_t i = index; i + 1 < m_threadCount; i++)
        m_readyQueue[i] = m_readyQueue[i + 1];

    m_threadCount--;
}

void Scheduler::switchTo(uint32_t nextIndex)
{
    TCB* from = m_readyQueue[m_currentThreadIndex];
    TCB* to = m_readyQueue[nextIndex];

    // An exited thread leaves the queue here. Its stack is still in use
    // until switchTask is done, the owner frees it later.
    if (from->state == ThreadState::Terminated) {
        removeAt(m_currentThreadIndex);
        if (nextIndex > m_currentThreadIndex)
            nextIndex--;
    } else if (from->state == ThreadState::Running) {
        from->state = ThreadState::Ready;
    }

    to->state = ThreadState::Running;
    m_currentThreadIndex = nextIndex;
    s_currentTask = to;

    // Interrupts and system calls from ring 3 land on the thread's own kernel stack
    if (to->stackBase)
        GDT::setKernelStack(to->stackBase + to->stackSize);

    switchTask(&from->regs, &to->regs);
}

// Switch to the next runnable thread after the current one. If the current
// thread is the only runnable one, keep running it. If nothing can run,
// halt until an interrupt (e.g. the timer waking a sleeper) changes that.
void Scheduler::schedule()
{
    // A thread may wait with interrupts off (Sync::WaitQueue); idling turns
    // them on, so put back what the caller had before returning to it
    bool interruptsWereEnabled = IOPort::interruptsEnabled();

    while (true) {
        wakeSleepers();

        for (uint32_t k = 1; k < m_threadCount; k++) {
            uint32_t index = (m_currentThreadIndex + k) % m_threadCount;
            if (isRunnable(m_readyQueue[index])) {
                switchTo(index);
                return;
            }
        }

        // Keep running the current thread; after a sleep it is only Ready
        if (isRunnable(m_readyQueue[m_currentThreadIndex])) {
            m_readyQueue[m_currentThreadIndex]->state = ThreadState::Running;
            return;
        }

        asm volatile("sti; hlt");
        if (!interruptsWereEnabled)
            IOPort::disableInterrupts();
    }
}

void Scheduler::addTask(Thread& thread)
{
    if (m_threadCount < maxTasks) {
        thread.getTCB().scheduler = this;
        m_readyQueue[m_threadCount++] = &thread.getTCB();
    } else {
        printf("Scheduler: run queue full, %s is not added\n", thread.getName());
    }
}

void Scheduler::removeTask(Thread& thread)
{
    TCB* tcb = &thread.getTCB();

    for (uint32_t i = 0; i < m_threadCount; i++) {
        if (m_readyQueue[i] != tcb)
            continue;

        if (i == m_currentThreadIndex)
            kernelPanic("Scheduler: a thread can not remove itself");

        removeAt(i);
        if (i < m_currentThreadIndex)
            m_currentThreadIndex--;
        return;
    }
}

void Scheduler::yield()
{
    schedule();
}

void Scheduler::sleep(uint64_t ms)
{
    if (ms == 0) {
        yield();
        return;
    }

    TCB* self = m_readyQueue[m_currentThreadIndex];
    self->wakeTick = m_ticks.load() + ms;
    self->state = ThreadState::Sleeping;
    schedule();
}

void Scheduler::block()
{
    m_readyQueue[m_currentThreadIndex]->state = ThreadState::Blocked;
    schedule();
}

void Scheduler::unblock(TCB& tcb)
{
    if (tcb.state == ThreadState::Blocked)
        tcb.state = ThreadState::Ready;
}

void Scheduler::exit()
{
    if (m_readyQueue[m_currentThreadIndex] == &m_bootTcb)
        kernelPanic("Scheduler: the boot flow can not exit");

    m_readyQueue[m_currentThreadIndex]->state = ThreadState::Terminated;
    schedule();

    // A terminated thread is never picked again
    kernelPanic("Scheduler: exited thread was resumed");
}

TCB& Scheduler::current()
{
    return *m_readyQueue[m_currentThreadIndex];
}

} // namespace Threading
