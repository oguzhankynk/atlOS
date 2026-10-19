/**
 * @file thread.cpp
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

#include <Thread/thread.hpp>
#include <stdio.h>
#include <string.h>

#include <Memory/VirtualMemory.hpp>
#include <Memory/KHeap.h>
#include <Scheduler/scheduler.hpp>

namespace Threading
{
    static uint16_t s_TaskIdCounter{};

    class ThreadImpl
    {
    public:
        ThreadImpl(EntryPoint entry);                               // Constructor to initialize the thread
        TCB m_tcb{};                                                // Thread Control Block (TCB) to store thread context
        EntryPoint m_entry{};                                       // Entry point function for the thread
        char m_name[16]{};                                          // Thread name (for debugging purposes)
        static constexpr size_t STACK_SIZE = Memory::PAGE_SIZE_2MB; // Stack size for the thread
        uint8_t m_stack[STACK_SIZE];                                // Stack for the thread
        static void threadStartPoint(void *obj);                    // Static function to start the thread
    };

    ThreadImpl::ThreadImpl(EntryPoint entry)
        : m_entry{entry}
    {
        snprintf(m_name, 16, "t%d", s_TaskIdCounter++);

          // Set up the stack
        memset(m_stack, 0x00, STACK_SIZE);

        // Set up the TCB (Thread Control Block)
        m_tcb.state = ThreadState::READY;
        m_tcb.regs.rax = 0;
        m_tcb.regs.rbx = 0;
        m_tcb.regs.rcx = 0;
        m_tcb.regs.rdx = 0;

        // In x86_64 System V ABI, the first argument goes to RDI
        m_tcb.regs.rdi = reinterpret_cast<uint64_t>(this);

        // Get RFLAGS and CR3 (64-bit versions)
        asm volatile("pushfq; popq %0" : "=r"(m_tcb.regs.rflags));
        asm volatile("movq %%cr3, %0" : "=r"(m_tcb.regs.cr3));

        // Instruction Pointer points to our static wrapper
        m_tcb.regs.rip = reinterpret_cast<uint64_t>(threadStartPoint);

        // Pointer to the stack
        //  The stack grows downwards, so we need to set the stack pointer to the top of the stack
        uint8_t *sp = m_stack + STACK_SIZE;

        auto push = [](uint8_t *&sp, auto value)
        {
            using Type = decltype(value);
            sp -= sizeof(Type);
            *reinterpret_cast<Type *>(sp) = value;
        };

        // Push the return address (the next instruction after the call to threadStartPoint)
        // Return address is never used
        push(sp, reinterpret_cast<uintptr_t>(nullptr));

        m_tcb.regs.rsp = reinterpret_cast<uint64_t>(sp);
        m_tcb.regs.rbp = m_tcb.regs.rsp;
    }

    void ThreadImpl::threadStartPoint(void *obj)
    {
        // Cast the object pointer back to ThreadImpl
        auto *thread = static_cast<ThreadImpl *>(obj);
        printf("Thread %s is running.\n", thread->m_name);

        // Set the thread state to RUNNING
        thread->m_tcb.state = ThreadState::RUNNING;

        // Call the entry point function
        thread->m_entry();

        // Thread has finished execution, set state to TERMINATED
        printf("Thread %s finished execution.\n", thread->m_name);

        thread->m_tcb.state = ThreadState::TERMINATED;

        while (true)
        {
            Scheduler::yield();
        }
    }

    Thread::Thread(EntryPoint entry, bool addToScheduler)
        : m_impl{new ThreadImpl(entry)}
    {
        if (addToScheduler)
        {
            Scheduler::addTask(*this);
        }
    }

    Thread::~Thread()
    {
        // Destructor to clean up resources
        printf("Thread %s dtor called.\n", m_impl->m_name);
        delete m_impl;
    }

    TCB &Thread::getTCB()
    {
        return m_impl->m_tcb;
    }

    const char *Thread::getName() const
    {
        return m_impl->m_name;
    }

} // namespace Thread