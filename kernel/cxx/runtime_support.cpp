/**
 * @file runtime_support.cpp
 * @author Oguzhan KAYNAK
 * @brief Contains runtime support functions
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

#include <stddef.h>
#include <stdlib.h>

/**
 * @brief Handles pure virtual function calls in C++.
 *
 * This function is called when a pure virtual function is invoked.
 * It outputs an error message to stderr and aborts the program to handle
 * the erroneous call.
 *
 * @return This function does not return any value. It aborts the program.
 */
extern "C" void __cxa_pure_virtual()
{
    abort();
}

namespace __cxxabiv1 {
/* guard variables */

/* The ABI requires a 64-bit type.  */
__extension__ typedef int __guard __attribute__((mode(__DI__)));

extern "C" int __cxa_guard_acquire(__guard*);
extern "C" void __cxa_guard_release(__guard*);
extern "C" void __cxa_guard_abort(__guard*);

extern "C" int __cxa_guard_acquire(__guard* g)
{
    return !*(char*)(g);
}

extern "C" void __cxa_guard_release(__guard* g)
{
    *(char*)g = 1;
}

extern "C" void __cxa_guard_abort(__guard*)
{
}

}