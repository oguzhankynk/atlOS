/**
 * @file cpuid.h
 * @author okaynak ()
 * @brief Header file defining the CPUID function for retrieving processor information.
 *  The CPUID function allows querying the CPU for various types of information.
 *  This header file declares a function that calls the CPUID instruction and stores the results.
 * @date 2024-03-31
 * 
 * @copyright Copyright (c) 2024
 * 
 */

#ifndef D417B4C4_41F9_4AFE_B052_9EC88867FCC8
#define D417B4C4_41F9_4AFE_B052_9EC88867FCC8

#if defined(__cplusplus)
extern "C" {
#endif

/// @brief Executes the CPUID instruction with the specified value and stores the results in the provided pointers.
/// @param value The value to be placed in the EAX register before executing the CPUID instruction.
/// @param eax Pointer to store the value in the EAX register after executing the CPUID instruction.
/// @param ebx Pointer to store the value in the EBX register after executing the CPUID instruction.
/// @param ecx Pointer to store the value in the ECX register after executing the CPUID instruction.
/// @param edx Pointer to store the value in the EDX register after executing the CPUID instruction.
void cpuid(unsigned int value, unsigned int *eax, unsigned int *ebx, unsigned int *ecx, unsigned int *edx);



#if defined(__cplusplus)
} /* extern "C" */
#endif

#endif /* D417B4C4_41F9_4AFE_B052_9EC88867FCC8 */
