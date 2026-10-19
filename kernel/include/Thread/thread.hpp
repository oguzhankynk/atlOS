/**
 * @file thread.hpp
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

#ifndef BE0C6057_D75F_4A2E_807F_8CFDF75C8093
#define BE0C6057_D75F_4A2E_807F_8CFDF75C8093

#include <stdint.h>

namespace Threading
{
    enum class ThreadState
    {
        RUNNING,
        READY,
        BLOCKED,  // Waits for Scheduler::unblock
        SLEEPING, // Waits until TCB::wakeTick
        TERMINATED
    };

    struct Registers
    {
        uint64_t r15, r14, r13, r12, r11, r10, r9, r8;
        uint64_t rbp, rdi, rsi, rdx, rcx, rbx, rax;

        uint64_t rip; // Instruction Pointer
        uint64_t cs;  // Code Segment
        uint64_t rflags;
        uint64_t rsp; // Stack Pointer
        uint64_t ss;  // Stack Segment

        uint64_t cr3;
    } __attribute__((packed));

    struct TCB
    {
        Registers regs; // Must stay first, switchTask takes &regs
        ThreadState state;
        uint64_t wakeTick; // Scheduler::ticks() value to leave SLEEPING at
    };

    class ThreadImpl;

    using EntryPoint = void (*)(void);

    class Thread
    {
    public:
        Thread(EntryPoint entry, bool addToScheduler = true);

        Thread(const Thread &) = delete;

        ~Thread();

        Thread &operator=(const Thread &) = delete;

        TCB &getTCB();

        const char *getName() const;

    private:
        ThreadImpl *m_impl{};
    };

} // namespace Thread
#endif /* BE0C6057_D75F_4A2E_807F_8CFDF75C8093 */
