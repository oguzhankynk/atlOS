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

#include <Memory/KHeap.h>
#include <Schduler/schduler.hpp>

namespace Threading
{
    static uint16_t s_TaskIdCounter{};

    class ThreadImpl
    {
    public:
        ThreadImpl(EntryPoint entry); // Constructor to initialize the thread
        TCB m_tcb{}; // Thread Control Block (TCB) to store thread context
        EntryPoint m_entry{}; // Entry point function for the thread
        char m_name[16]{}; // Thread name (for debugging purposes)
        static constexpr size_t STACK_SIZE = 4096; // Stack size for the thread
        uint8_t stack[STACK_SIZE]; // Stack for the thread
        static void threadStartPoint(void* obj); // Static function to start the thread
    };

    ThreadImpl::ThreadImpl(EntryPoint entry)
        :
        m_tcb{Registers{}, ThreadState::READY},
        m_entry{ entry }
    {
        snprintf(m_name, 16, "t%d", s_TaskIdCounter++);

        // Set up the stack
        memset(stack, 0x00, STACK_SIZE);

        // Set up the TCB (Thread Control Block)
        m_tcb.regs.eax = 0;
        m_tcb.regs.ebx = 0;
        m_tcb.regs.ecx = 0;
        m_tcb.regs.edx = 0;
        m_tcb.regs.esi = 0;
        m_tcb.regs.edi = 0;

        // get the EFLAGS and CR3 registers
        asm volatile("pushfl; movl (%%esp), %%eax; movl %%eax, %0; popfl;":"=m"(m_tcb.regs.eflags)::"%eax");
        asm volatile("movl %%cr3, %%eax; movl %%eax, %0;":"=m"(m_tcb.regs.cr3)::"%eax");

        // Set up the instruction pointer
        m_tcb.regs.eip = reinterpret_cast<uintptr_t>(threadStartPoint);

        //Pointer to the stack
        // The stack grows downwards, so we need to set the stack pointer to the top of the stack
        uintptr_t* sp = reinterpret_cast<uintptr_t*>(stack + STACK_SIZE );
        
        //push the first argument to the stack (the object itself)
        --sp;  
        *sp = reinterpret_cast<uintptr_t>(this); 

        //push the return address (the next instruction after the call to threadStartPoint)
        //return address is never used
        --sp;  
        *sp = reinterpret_cast<uintptr_t>(nullptr); 
        
        m_tcb.regs.esp = reinterpret_cast<uintptr_t>(sp);
    }


    void ThreadImpl::threadStartPoint(void* obj)
    {
        // Cast the object pointer back to ThreadImpl
        auto* thread = static_cast<ThreadImpl*>(obj);
        printf("Thread %s is running.\n", thread->m_name);
        
        // Set the thread state to RUNNING
        thread->m_tcb.state = ThreadState::RUNNING;

        // Call the entry point function
        thread->m_entry();
    
        // Thread has finished execution, set state to TERMINATED
        printf("Thread %s finished execution.\n", thread->m_name);

        thread->m_tcb.state = ThreadState::TERMINATED;


        Schedular::yield();
    }

    Thread::Thread(EntryPoint entry, bool addToScheduler)
        : 
        m_impl{ new ThreadImpl(entry) }
    {
        if (addToScheduler)
        {
            Schedular::addTask(*this);
        }
    }

    Thread::~Thread()
    {
        // Destructor to clean up resources
        printf("Thread %s dtor called.\n", m_impl->m_name);
        delete m_impl;
    }

    TCB& Thread::getTCB()
    {
        return m_impl->m_tcb;
    }

    const char* Thread::getName() const
    {
        return m_impl->m_name;
    }


} // namespace Thread