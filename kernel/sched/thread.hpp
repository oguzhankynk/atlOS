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

#pragma once

#include <stdint.h>

namespace Memory {
class AddressSpace;
}

namespace Threading {
enum class ThreadState {
    Running,
    Ready,
    Blocked, // Waits for Scheduler::unblock
    Sleeping, // Waits until TCB::wakeTick
    Terminated
};

struct Registers {
    uint64_t r15, r14, r13, r12, r11, r10, r9, r8;
    uint64_t rbp, rdi, rsi, rdx, rcx, rbx, rax;

    uint64_t rip; // Instruction Pointer
    uint64_t cs; // Code Segment
    uint64_t rflags;
    uint64_t rsp; // Stack Pointer
    uint64_t ss; // Stack Segment

    uint64_t cr3;
} __attribute__((packed));

class Scheduler;

struct TCB {
    Registers regs; // Must stay first, switchTask takes &regs
    ThreadState state;
    uint64_t wakeTick; // Scheduler::ticks() value to leave Sleeping at

    // For task listings (the shell's "tasks")
    const char* name;
    uintptr_t stackBase; // 0: not a Thread's stack (the boot flow)
    uint64_t stackSize;

    void* owner; // the user program this thread runs, nullptr for kernel threads

    Scheduler* scheduler; // the scheduler that runs this task (set when it is queued)
};

class ThreadImpl;

/// A thread's code. It reaches its scheduler through Threading::yield(),
/// Threading::sleep(), ... (the running task's scheduler).
using EntryPoint = void (*)();

class Thread {
public:
    /**
     * @param scheduler Runs the thread; must outlive it.
     * @param addressSpace Where the thread's 2 MB stack page is allocated; must
     *                     outlive the thread.
     * @param addToScheduler Queue the thread right away.
     */
    Thread(Scheduler& scheduler, Memory::AddressSpace& addressSpace, EntryPoint entry, bool addToScheduler = true);

    /**
     * @brief A thread on the caller's scheduler (Threading::currentScheduler())
     * with its stack in the kernel address space (Memory::kernelAddressSpace()).
     */
    explicit Thread(EntryPoint entry, bool addToScheduler = true);

    Thread(const Thread&) = delete;

    ~Thread();

    Thread& operator=(const Thread&) = delete;

    TCB& getTCB();

    const char* getName() const;

private:
    ThreadImpl* m_impl {};
};

} // namespace Thread
