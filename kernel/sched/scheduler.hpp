/**
 * @file scheduler.hpp
 * @author Oguzhan KAYNAK ()
 * @brief Cooperative round robin scheduler.
 *
 * Threads only change on yield/sleep/block/exit; the timer interrupt just
 * counts ticks and never switches threads.
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

namespace Threading {
class Thread;
struct TCB;
};

namespace Scheduler {
/**
 * @brief Register the boot flow as the first task and start the 1 ms tick.
 * Needs the PIC to be initialized.
 */
void init();

void addTask(Threading::Thread& thread);

/**
 * @brief Remove a thread from the run queue. No-op if it is not queued
 * (e.g. it has already exited). The running thread can not remove itself.
 */
void removeTask(Threading::Thread& thread);

/**
 * @brief Run the next runnable thread, if there is one.
 */
void yield();

/**
 * @brief Let other threads run for at least ms milliseconds.
 */
void sleep(uint64_t ms);

/**
 * @brief Stop running the current thread until someone calls unblock on it.
 */
void block();

/**
 * @brief Make a Blocked thread runnable again. Other states are left alone.
 */
void unblock(Threading::TCB& tcb);

/**
 * @brief End the current thread. It leaves the run queue and never runs again;
 * its owner still has to delete the Thread object.
 */
[[noreturn]] void exit();

/**
 * @brief TCB of the running thread.
 */
Threading::TCB& current();

/**
 * @brief Milliseconds since Init.
 */
uint64_t ticks();

/**
 * @brief Number of queued tasks, including the boot flow.
 */
uint32_t taskCount();

} // namespace Scheduler
