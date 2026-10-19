#include <Scheduler/scheduler.hpp>
#include <Thread/thread.hpp>
#include <stdio.h>

extern "C" void switchTask(Threading::Registers *from, Threading::Registers *to);

namespace Scheduler
{
    static constexpr uint32_t MAX_TASKS = 16;

    // Context of the boot flow (kernelMain). It has no Thread object, but it
    // must own a TCB so that the first switchTask has somewhere to save it.
    static Threading::TCB bootTCB{};

    static Threading::TCB *readyQueue[MAX_TASKS];
    static uint32_t threadCount = 0;
    static uint32_t currentThreadIndex = 0;

    void Init() {
        bootTCB.state = Threading::ThreadState::RUNNING;
        readyQueue[0] = &bootTCB;
        threadCount = 1;
        currentThreadIndex = 0;
    }

    void addTask(Threading::Thread &thread) {
        if (threadCount < MAX_TASKS) {
            readyQueue[threadCount++] = &thread.getTCB();
        }
    }

    void yield() {
        if (threadCount <= 1) return;

        uint32_t prevIndex = currentThreadIndex;
        currentThreadIndex = (currentThreadIndex + 1) % threadCount;

        Threading::TCB* from = readyQueue[prevIndex];
        Threading::TCB* to = readyQueue[currentThreadIndex];

        if (from != to) {
            switchTask(&from->regs, &to->regs);
        }
    }
}
