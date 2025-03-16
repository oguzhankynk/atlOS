/**
 * @file schduler.cpp
 * @author Oguzhan KAYNAK ()
 * @brief
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

#include "Schduler/schduler.hpp"

#include "stdio.h"

#include "Thread/thread.hpp"
#include "kernel/sys/IOPort.hpp"
#include "kernel/assert.h"
#include <kernel/Logger.hpp>

 /**
  * @brief Architecture spesific context switch. Defined at arch/x/context.S
  *
  * @param currentThread
  * @param nextThread
  */
extern "C" void switchCPUContext(Threading::TCB* currentThread, Threading::TCB* nextThread);

namespace Schedular
{
    static constexpr LogFlag DBG_LOG = LogFlag::Log;
    static constexpr LogFlag INF_LOG = LogFlag::Log;
    static constexpr LogFlag ERR_LOG = LogFlag::Log;

    static uint16_t currThreadIndex{};
    static uint16_t threadCount{};

    static constexpr size_t MAX_THREAD = 255;

    static Threading::Thread* ThreadList[MAX_THREAD]{};

    static void busyWait() {
        for (volatile int i = 0; i < 1000000; i++) {
            // Zaman harcamak için bekleme (Busy wait)
        }
    }

    static void IdleThread()
    {
          while (true) {
            busyWait();    // Zamanı doldurmak için bekleme
            printf( "IdleThread calisiyor\n");
            yield();    // Diğer task'lere geçiş yap
        }

    }
    static void exampleTask1() {
        while (true) {
            busyWait();
            printf( "Task 1 calisiyor\n");
            yield(); 
        }
    }
    
    static void exampleTask2() {
        while (true) {
            busyWait();
            printf( "Task 2 calisiyor\n");
            yield();
        }
    }

    void Init()
    {
        ThreadList[0] = new Threading::Thread(IdleThread);
        ThreadList[1] = new Threading::Thread(exampleTask1);
        ThreadList[2] = new Threading::Thread(exampleTask2);


        currThreadIndex = 0;
        threadCount = 3;

        Threading::TCB tempTCB{};

        switchCPUContext(&tempTCB, &(ThreadList[0]->getTCB()));

    }

    void addTask(Threading::Thread& thread)
    {
        for (auto& threadSlot : ThreadList)
        {
            if (threadSlot == nullptr)
            {
                threadSlot = &thread;
                threadCount++;
                return;
            }
        }
        kernel_panic("Maximum thread number is reached \n");
    }

    void switchToThread(Threading::Thread& nextThread,  uint32_t indx)
    {
        auto currThread = ThreadList[currThreadIndex];
        currThreadIndex = indx;
        switchCPUContext(&currThread->getTCB(), &nextThread.getTCB());

        printf("geri geldim. currThreadIndex:%d name:%s \n", currThreadIndex, ThreadList[currThreadIndex]->getName());
        printf("esp:%x eip:%x \n", ThreadList[currThreadIndex]->getTCB().esp, ThreadList[currThreadIndex]->getTCB().eip);
    }

    void yield()
    {

        const auto currThread = ThreadList[currThreadIndex];

        auto indx = (currThreadIndex + 1) % MAX_THREAD;

        LOG_MSG(DBG_LOG, "curr task-> name:%s indx: %u \n", currThread->getName(), currThreadIndex);

        for (;;)
        {
            auto& thread = ThreadList[indx];

            if (thread != nullptr)
            {
                if (thread != currThread)
                {
                    LOG_MSG(DBG_LOG, "switch to task-> name:%s indx: %u \n", thread->getName(), indx);
                    
                    switchToThread(*thread, indx);

                    
                }
                break;
            }
            else
            {
                indx = (indx + 1) % MAX_THREAD;
            }
        }

    }

}// namespace Schedular