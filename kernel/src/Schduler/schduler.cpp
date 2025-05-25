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
#include <Memory/KHeap.h>

extern "C" void switchTask(Threading::Registers *old_regs, Threading::Registers *new_regs); // The function which actually switches

// extern "C" void switchTask(void* old_regs, void* new_regs); // The function which actually switches

namespace Schedular
{
    static void idleThreadFunction();

    class ThreadList
    {
    public:
        struct Node
        {
            Threading::Thread *thread{};
            Node *next{};
        };

    private:
        Node *m_head{};
        Node *m_tail{};
        Node *m_current{};
        uint32_t m_threadCount{};

    public:
        ThreadList()
        {
            auto idleThread = new Node();
            idleThread->thread = new Threading::Thread(idleThreadFunction, false);

            m_head = idleThread;
            m_tail = idleThread;
            m_current = idleThread;
            m_tail->next = m_head;
            m_threadCount = 1;
        }

        void addThread(Threading::Thread &thread)
        {
            auto newNode = new Node();
            newNode->thread = &thread;
            m_tail->next = newNode;
            m_tail = newNode;
            m_tail->next = m_head;
            m_threadCount++;
        }

        void removeThread(Threading::Thread &thread)
        {
            Node *current = m_head;
            Node *prev = m_tail;
            m_threadCount--;
            while (current != m_tail)
            {
                if (current->thread == &thread)
                {
                    prev->next = current->next;

                    return;
                }
                prev = current;
                current = current->next;
            }
        }

        Node *switchToNextThread(Node *&current)
        {
            current = m_current;
            while (true)
            {
                m_current = m_current->next;
                if (m_current->thread->getTCB().state == Threading::ThreadState::TERMINATED)
                {
                    continue; // Skip terminated threads
                }
                else
                {
                    break; // Found a thread that is not terminated
                }
            }
            return m_current;
        }

        uint32_t getThreadCount() const
        {
            return m_threadCount;
        }

        void controlThreads()
        {
            Node *current = m_head;

            printf("controlThreads: Thread count: %d\n", m_threadCount);

            for (uint32_t i = 0; i < m_threadCount; ++i)
            {
                if (current->thread->getTCB().state == Threading::ThreadState::TERMINATED)
                {
                    printf("Thread %s will be terminated.\n", current->thread->getName());
                    removeThread(*current->thread);
                    delete current->thread; // Free the thread object
                }

                current = current->next;
            }
        }

        Node *getCurrentThread()
        {
            return m_current;
        }
    };
 
    static ThreadList *threadList = nullptr;

    static void sleep(uint32_t ms)
    {
        for (uint32_t i = 0; i < ms * 1000; ++i)
        {
            asm volatile("nop");
        }
    }

    static void idleThreadFunction()
    {
        while (1)
        {
            printf("idleThreadFunction!\n"); // Not implemented here...
            threadList->controlThreads();
            yield();
            sleep(10000000);
        }
    }

    void helloWorldThreadFunction()
    {
        printf("Hello from helloWorldThreadFunction!\n");
        yield();
        printf("Hello again from helloWorldThreadFunction!\n");
    }

    void yield()
    {
        ThreadList::Node *last = nullptr;
        auto nextThread = threadList->switchToNextThread(last);

        switchTask(&last->thread->getTCB().regs, &nextThread->thread->getTCB().regs);
    }

    void Init()
    {
        threadList = new ThreadList();

        auto idleThread = threadList->getCurrentThread();

        auto newThread = new Threading::Thread(helloWorldThreadFunction, false);
        addTask(*newThread);

        Threading::Registers temp;
        switchTask(&temp, &idleThread->thread->getTCB().regs);
    }

    void addTask(Threading::Thread &thread)
    {
        threadList->addThread(thread);
    }

} // namespace Schedular