#include <Schduler/schduler.hpp>
#include <Thread/thread.hpp>
#include <stdio.h>

extern "C" void switchTask(Threading::Registers *from, Threading::Registers *to);

namespace Schedular
{
    static Threading::Thread* readyQueue[16];
    static uint32_t threadCount = 0;
    static uint32_t currentThreadIndex = 0;

    void Init() {
        threadCount = 0;
        currentThreadIndex = 0;
    }

    void addTask(Threading::Thread &thread) {
        if (threadCount < 16) {
            readyQueue[threadCount++] = &thread;
        }
    }

    void yield() {
        if (threadCount <= 1) return;

        uint32_t prevIndex = currentThreadIndex;
        currentThreadIndex = (currentThreadIndex + 1) % threadCount;

        Threading::Thread* from = readyQueue[prevIndex];
        Threading::Thread* to = readyQueue[currentThreadIndex];

        if (from != to) {
            switchTask(&from->getTCB().regs, &to->getTCB().regs);
        }
    }
}
