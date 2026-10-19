#include <core/panic.hpp>
#include <drivers/pit.hpp>
#include <sched/scheduler.hpp>
#include <sched/thread.hpp>
#include <stdio.h>

extern "C" void switchTask(Threading::Registers* from, Threading::Registers* to);

namespace Scheduler {
using Threading::TCB;
using Threading::ThreadState;

static constexpr uint32_t maxTasks = 16;
static constexpr uint32_t tickPeriodNs = 1000000; // 1 ms

// Context of the boot flow (kernelMain). It has no Thread object, but it
// must own a TCB so that the first switchTask has somewhere to save it.
static TCB s_bootTcb {};

static TCB* s_readyQueue[maxTasks];
static uint32_t s_threadCount = 0;
static uint32_t s_currentThreadIndex = 0;

// Only written by the timer interrupt; an aligned 64-bit store is atomic
static volatile uint64_t s_ticks = 0;

static void onTick(void*)
{
    s_ticks = s_ticks + 1;
}

static bool isRunnable(const TCB* tcb)
{
    return tcb->state == ThreadState::Ready || tcb->state == ThreadState::Running;
}

static void wakeSleepers()
{
    uint64_t now = s_ticks;
    for (uint32_t i = 0; i < s_threadCount; i++) {
        if (s_readyQueue[i]->state == ThreadState::Sleeping && now >= s_readyQueue[i]->wakeTick)
            s_readyQueue[i]->state = ThreadState::Ready;
    }
}

static void removeAt(uint32_t index)
{
    for (uint32_t i = index; i + 1 < s_threadCount; i++)
        s_readyQueue[i] = s_readyQueue[i + 1];

    s_threadCount--;
}

static void switchTo(uint32_t nextIndex)
{
    TCB* from = s_readyQueue[s_currentThreadIndex];
    TCB* to = s_readyQueue[nextIndex];

    // An exited thread leaves the queue here. Its stack is still in use
    // until switchTask is done, the owner frees it later.
    if (from->state == ThreadState::Terminated) {
        removeAt(s_currentThreadIndex);
        if (nextIndex > s_currentThreadIndex)
            nextIndex--;
    } else if (from->state == ThreadState::Running) {
        from->state = ThreadState::Ready;
    }

    to->state = ThreadState::Running;
    s_currentThreadIndex = nextIndex;

    switchTask(&from->regs, &to->regs);
}

// Switch to the next runnable thread after the current one. If the current
// thread is the only runnable one, keep running it. If nothing can run,
// halt until an interrupt (e.g. the timer waking a sleeper) changes that.
static void schedule()
{
    while (true) {
        wakeSleepers();

        for (uint32_t k = 1; k < s_threadCount; k++) {
            uint32_t index = (s_currentThreadIndex + k) % s_threadCount;
            if (isRunnable(s_readyQueue[index])) {
                switchTo(index);
                return;
            }
        }

        if (isRunnable(s_readyQueue[s_currentThreadIndex]))
            return;

        asm volatile("sti; hlt");
    }
}

void init()
{
    s_bootTcb.state = ThreadState::Running;
    s_readyQueue[0] = &s_bootTcb;
    s_threadCount = 1;
    s_currentThreadIndex = 0;

    PIT::init(tickPeriodNs, onTick);
}

void addTask(Threading::Thread& thread)
{
    if (s_threadCount < maxTasks) {
        s_readyQueue[s_threadCount++] = &thread.getTCB();
    } else {
        printf("Scheduler: run queue full, %s is not added\n", thread.getName());
    }
}

void removeTask(Threading::Thread& thread)
{
    TCB* tcb = &thread.getTCB();

    for (uint32_t i = 0; i < s_threadCount; i++) {
        if (s_readyQueue[i] != tcb)
            continue;

        if (i == s_currentThreadIndex)
            kernelPanic("Scheduler: a thread can not remove itself");

        removeAt(i);
        if (i < s_currentThreadIndex)
            s_currentThreadIndex--;
        return;
    }
}

void yield()
{
    schedule();
}

void sleep(uint64_t ms)
{
    if (ms == 0) {
        yield();
        return;
    }

    TCB* self = s_readyQueue[s_currentThreadIndex];
    self->wakeTick = s_ticks + ms;
    self->state = ThreadState::Sleeping;
    schedule();
}

void block()
{
    s_readyQueue[s_currentThreadIndex]->state = ThreadState::Blocked;
    schedule();
}

void unblock(TCB& tcb)
{
    if (tcb.state == ThreadState::Blocked)
        tcb.state = ThreadState::Ready;
}

void exit()
{
    if (s_readyQueue[s_currentThreadIndex] == &s_bootTcb)
        kernelPanic("Scheduler: the boot flow can not exit");

    s_readyQueue[s_currentThreadIndex]->state = ThreadState::Terminated;
    schedule();

    // A terminated thread is never picked again
    kernelPanic("Scheduler: exited thread was resumed");
    while (true) {
    }
}

TCB& current()
{
    return *s_readyQueue[s_currentThreadIndex];
}

uint64_t ticks()
{
    return s_ticks;
}

uint32_t taskCount()
{
    return s_threadCount;
}
}
