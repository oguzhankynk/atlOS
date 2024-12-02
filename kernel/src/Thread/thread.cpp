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

namespace Threading
{
    static uint16_t s_TaskIdCounter{};

    

    class ThreadImpl
    {
    public:
        ThreadImpl(EntryPoint entry);
        ThreadState m_state{};
        TCB m_tcb{};
        EntryPoint m_entry{};
        char m_name[16]{};
        uint8_t stack[4096];
    };

    ThreadImpl::ThreadImpl(EntryPoint entry)
        :m_state{ ThreadState::READY },
        m_tcb{},
        m_entry{ entry }
    {
        snprintf(m_name, 16, "t%d", s_TaskIdCounter++);

        m_tcb.esp = reinterpret_cast<uint32_t>(stack + 4092); 
        m_tcb.ebp = 0;                         

        m_tcb.esp -= sizeof(uint32_t);        
        *reinterpret_cast<uint32_t*>(m_tcb.esp) = reinterpret_cast<uint32_t>(m_entry);  

        m_tcb.ebx = 0;
        m_tcb.esi = 0;
        m_tcb.edi = 0;
        m_tcb.eip = reinterpret_cast<uint32_t>(m_entry); 
        m_tcb.eflags = 0x202;     
        m_tcb.cr3 = 0;            
    
        printf("%s: spawned: f:%x esp:%x eip:%x \n",m_name, m_entry, m_tcb.esp, m_tcb.eip);
    }

    Thread::Thread(EntryPoint entry)
        :
        m_impl{ new ThreadImpl(entry) }
    {

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