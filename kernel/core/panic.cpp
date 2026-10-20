/**
 * @file panic.cpp
 * @author Oguzhan KAYNAK
 * @brief Handles kernel panic situations.
 *
 * This file contains the `kernelPanic` function, which is responsible for handling critical errors
 * that may occur in the kernel and halting the system gracefully while displaying warnings.
 *
 * Copyright © 2024 Oguzhan KAYNAK
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the “Software”), to deal in the Software
 * without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit
 * persons to whom the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
 * PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include <core/panic.hpp>
#include <stdio.h>

#include <arch/x86_64/cpu/backtrace.hpp>
#include <core/halt.hpp>

// Set by the first panic: a fault while panicking (e.g. a broken frame chain
// in the backtrace) must not start the report again, maybe forever
static bool s_panicking = false;

void kernelPanicFrom(const char* str, uint64_t rip, uint64_t rbp)
{
    __asm__ volatile("cli");

    if (s_panicking) {
        printf("\n==== PANIC WHILE PANICKING: %s ====\n", str);
        Core::haltWithStatus(Core::ExitStatus::Panic);
    }
    s_panicking = true;

    printf("\n==== KERNEL PANIC ====\n%s\n", str);
    Backtrace::print(rip, rbp);
    Core::haltWithStatus(Core::ExitStatus::Panic);
}

void kernelPanic(const char* str)
{
    // Start at the caller: our frame holds its rbp, and we return into it
    uint64_t rbp = *static_cast<uint64_t*>(__builtin_frame_address(0));
    uint64_t rip = reinterpret_cast<uint64_t>(__builtin_return_address(0));
    kernelPanicFrom(str, rip, rbp);
}

void kernelPanicAssert(bool condition)
{
    if (!condition) {
        kernelPanic("ASSERTION-FAILED");
    }
}
