/**
 * @file panic.hpp
 * @author Oguzhan KAYNAK
 * @brief Declaration of kernel panic situations function.
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
#pragma once

#if defined(__cplusplus)
extern "C" {
#endif

/**
 * @brief Kernel panic function.
 *
 * @param str Message to be displayed in case of a kernel panic.
 * @return void
 */
void kernelPanic(const char* str);
void kernelPanicAssert(bool condition);

#if defined(__cplusplus)
} /* extern "C" */
#endif
