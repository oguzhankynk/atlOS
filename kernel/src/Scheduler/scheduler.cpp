#include <Scheduler/scheduler.hpp>
#include <Thread/thread.hpp>
#include <hardware/pit.hpp>
#include <kernel/panic.h>
#include <stdio.h>

extern "C" void switchTask(Threading::Registers *from, Threading::Registers *to);

namespace Scheduler
{
    using Threading::TCB;
    using Threading::ThreadState;

    static constexpr uint32_t MAX_TASKS = 16;
    static constexpr uint32_t TICK_PERIOD_NS = 1000000; // 1 ms

    // Context of the boot flow (kernelMain). It has no Thread object, but it
    // must own a TCB so that the first switchTask has somewhere to save it.
    static TCB bootTCB{};

    static TCB *readyQueue[MAX_TASKS];
    static uint32_t threadCount = 0;
    static uint32_t currentThreadIndex = 0;

    // Only written by the timer interrupt; an aligned 64-bit store is atomic
    static volatile uint64_t s_ticks = 0;

    static void onTick(void *)
    {
        s_ticks = s_ticks + 1;
    }

    static bool isRunnable(const TCB *tcb)
    {
        return tcb->state == ThreadState::READY || tcb->state == ThreadState::RUNNING;
    }

    static void wakeSleepers()
    {
        uint64_t now = s_ticks;
        for (uint32_t i = 0; i < threadCount; i++)
        {
            if (readyQueue[i]->state == ThreadState::SLEEPING && now >= readyQueue[i]->wakeTick)
                readyQueue[i]->state = ThreadState::READY;
        }
    }

    static void removeAt(uint32_t index)
    {
        for (uint32_t i = index; i + 1 < threadCount; i++)
            readyQueue[i] = readyQueue[i + 1];

        threadCount--;
    }

    static void switchTo(uint32_t nextIndex)
    {
        TCB *from = readyQueue[currentThreadIndex];
        TCB *to = readyQueue[nextIndex];

        // An exited thread leaves the queue here. Its stack is still in use
        // until switchTask is done, the owner frees it later.
        if (from->state == ThreadState::TERMINATED)
        {
            removeAt(currentThreadIndex);
            if (nextIndex > currentThreadIndex)
                nextIndex--;
        }
        else if (from->state == ThreadState::RUNNING)
        {
            from->state = ThreadState::READY;
        }

        to->state = ThreadState::RUNNING;
        currentThreadIndex = nextIndex;

        switchTask(&from->regs, &to->regs);
    }

    // Switch to the next runnable thread after the current one. If the current
    // thread is the only runnable one, keep running it. If nothing can run,
    // halt until an interrupt (e.g. the timer waking a sleeper) changes that.
    static void schedule()
    {
        while (true)
        {
            wakeSleepers();

            for (uint32_t k = 1; k < threadCount; k++)
            {
                uint32_t index = (currentThreadIndex + k) % threadCount;
                if (isRunnable(readyQueue[index]))
                {
                    switchTo(index);
                    return;
                }
            }

            if (isRunnable(readyQueue[currentThreadIndex]))
                return;

            asm volatile("sti; hlt");
        }
    }

    void Init()
    {
        bootTCB.state = ThreadState::RUNNING;
        readyQueue[0] = &bootTCB;
        threadCount = 1;
        currentThreadIndex = 0;

        PIT::init(TICK_PERIOD_NS, onTick);
    }

    void addTask(Threading::Thread &thread)
    {
        if (threadCount < MAX_TASKS)
        {
            readyQueue[threadCount++] = &thread.getTCB();
        }
        else
        {
            printf("Scheduler: run queue full, %s is not added\n", thread.getName());
        }
    }

    void removeTask(Threading::Thread &thread)
    {
        TCB *tcb = &thread.getTCB();

        for (uint32_t i = 0; i < threadCount; i++)
        {
            if (readyQueue[i] != tcb)
                continue;

            if (i == currentThreadIndex)
                kernel_panic("Scheduler: a thread can not remove itself");

            removeAt(i);
            if (i < currentThreadIndex)
                currentThreadIndex--;
            return;
        }
    }

    void yield()
    {
        schedule();
    }

    void sleep(uint64_t ms)
    {
        if (ms == 0)
        {
            yield();
            return;
        }

        TCB *self = readyQueue[currentThreadIndex];
        self->wakeTick = s_ticks + ms;
        self->state = ThreadState::SLEEPING;
        schedule();
    }

    void block()
    {
        readyQueue[currentThreadIndex]->state = ThreadState::BLOCKED;
        schedule();
    }

    void unblock(TCB &tcb)
    {
        if (tcb.state == ThreadState::BLOCKED)
            tcb.state = ThreadState::READY;
    }

    void exit()
    {
        if (readyQueue[currentThreadIndex] == &bootTCB)
            kernel_panic("Scheduler: the boot flow can not exit");

        readyQueue[currentThreadIndex]->state = ThreadState::TERMINATED;
        schedule();

        // A terminated thread is never picked again
        kernel_panic("Scheduler: exited thread was resumed");
        while (true)
        {
        }
    }

    TCB &current()
    {
        return *readyQueue[currentThreadIndex];
    }

    uint64_t ticks()
    {
        return s_ticks;
    }

    uint32_t taskCount()
    {
        return threadCount;
    }
}
