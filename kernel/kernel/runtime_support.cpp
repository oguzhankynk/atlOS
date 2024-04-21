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