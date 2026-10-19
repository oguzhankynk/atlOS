/**
 * @file spinlock.cpp
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

#include <sync/spinlock.hpp>

namespace Sync
{
    /**
     * @brief Acquire the lock, spin until it is available
     *
     */
    void Spinlock::lock()
    {

        while (true)
        {
            bool expected = false;
            // Atomically set m_locked to true if it was false (unlocked)
            if (__atomic_compare_exchange_n(
                    &m_locked,
                    &expected,
                    true,
                    false,
                    __ATOMIC_SEQ_CST,
                    __ATOMIC_SEQ_CST))
            {
                // Lock acquired successfully
                return;
            }
            // Hint to the processor that we are in a spin-wait loop
            asm volatile("pause");
        }
    }

    /**
     * @brief  Release the lock, allowing other threads to acquire it
     *
     */
    void Spinlock::unlock()
    {
        // Atomically set m_locked to false (unlocked) with sequential consistency.
        // This ensures that the lock is safely released and visible to all threads in the correct order.
        __atomic_store_n(&m_locked, false, __ATOMIC_SEQ_CST);
    }
} // namespace Sync