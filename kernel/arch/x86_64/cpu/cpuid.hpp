/**
 * @file cpuid.hpp
 * @author Oguzhan KAYNAK
 * @brief Header file defining the CPUID function for retrieving processor information.
 *  The CPUID function allows querying the CPU for various types of information.
 *  This header file declares a function that calls the CPUID instruction and stores the results.
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

#if defined(__cplusplus)
extern "C" {
#endif

/// @brief Executes the CPUID instruction with the specified value and stores the results in the provided pointers.
/// @param value The value to be placed in the EAX register before executing the CPUID instruction.
/// @param eax Pointer to store the value in the EAX register after executing the CPUID instruction.
/// @param ebx Pointer to store the value in the EBX register after executing the CPUID instruction.
/// @param ecx Pointer to store the value in the ECX register after executing the CPUID instruction.
/// @param edx Pointer to store the value in the EDX register after executing the CPUID instruction.
void cpuid(unsigned int value, unsigned int* eax, unsigned int* ebx, unsigned int* ecx, unsigned int* edx);

#if defined(__cplusplus)
} /* extern "C" */
#endif
