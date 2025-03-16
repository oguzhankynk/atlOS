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
        BLOCKED,
        TERMINATED
    };

    struct TCB
    {
        uint32_t esp{};    /// kernel stack pointer (ESP)
        uint32_t ebp{};    /// Stack frame pointer (EBP)
        uint32_t ebx{};    /// General purpose register (EBX)
        uint32_t esi{};    /// Source index register (ESI)
        uint32_t edi{};    /// Destination index register (EDI)
        uint32_t eip{};    /// Instruction pointer (EIP)
        uint32_t cr3{};    /// Page directory base address (CR3)
        uint32_t eflags{}; /// CPU flags (EFLAGS)
    } __attribute__((packed));;

    class ThreadImpl;

    using EntryPoint = void(*)(void);

    class Thread
    {
    public:
        
        Thread(EntryPoint entry);

        Thread(const Thread&) = delete;

        Thread& operator=(const Thread&) = delete;

        TCB& getTCB();

        const char* getName() const;
    private:
        ThreadImpl *m_impl{};
    };

} // namespace Thread
#endif /* BE0C6057_D75F_4A2E_807F_8CFDF75C8093 */
