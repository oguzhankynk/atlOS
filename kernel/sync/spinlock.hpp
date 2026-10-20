/**
 * @file spinlock.hpp
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
 *
 */

#pragma once

#include <sync/atomic.hpp>

namespace Sync {

/**
 * @brief Busy waiting lock, for several CPUs (SMP).
 *
 * On one CPU it only helps if used by the rules: never wait, yield or sleep
 * while holding it, and if an interrupt handler takes it too, hold it with
 * interrupts off (InterruptGuard). Otherwise the CPU spins forever on a lock
 * whose holder can not run again. See README.md.
 */
class Spinlock {
private:
    /**
     * @brief Whether the lock is currently held
     *
     */
    Atomic<bool> m_locked { false };

public:
    /**
     * @brief Acquire the lock, spin until it is available
     *
     */
    void lock();

    /**
     * @brief  Release the lock, allowing other threads to acquire it
     *
     */
    void unlock();
};
} // namespace Sync
