/**
 * @file panic.h
 * @author Oguzhan KAYNAK
 * @brief Declaration of kernel panic situations function.
 *
 * This file contains the `kernel_panic` function, which is responsible for handling critical errors
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
#ifndef EA38D7AA_B1BA_426B_9C15_22D380DBB786
#define EA38D7AA_B1BA_426B_9C15_22D380DBB786

#if defined(__cplusplus)
extern "C"
{
#endif

    /**
     * @brief Kernel panic function.
     *
     * @param str Message to be displayed in case of a kernel panic.
     * @return void
     */
    void kernel_panic(const char *str);
    void kernel_panic_assert(bool condition);

#if defined(__cplusplus)
} /* extern "C" */
#endif

#endif /* EA38D7AA_B1BA_426B_9C15_22D380DBB786 */
