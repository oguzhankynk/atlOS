/**
 * @file runtime_support.cpp
 * @author okaynak ()
 * @brief Contains runtime support functions
 * @date 2024-04-20
 * 
 * @copyright Copyright (c) 2024
 * 
 */

#include <stdlib.h> 
#include <stddef.h>

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

namespace __cxxabiv1 
{
	/* guard variables */
 
	/* The ABI requires a 64-bit type.  */
	__extension__ typedef int __guard __attribute__((mode(__DI__)));
 
	extern "C" int __cxa_guard_acquire (__guard *);
	extern "C" void __cxa_guard_release (__guard *);
	extern "C" void __cxa_guard_abort (__guard *);
 
	extern "C" int __cxa_guard_acquire (__guard *g) 
	{
		return !*(char *)(g);
	}
 
	extern "C" void __cxa_guard_release (__guard *g)
	{
		*(char *)g = 1;
	}
 
	extern "C" void __cxa_guard_abort (__guard *)
	{
 
	}
}