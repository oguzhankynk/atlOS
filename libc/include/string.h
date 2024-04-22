/**
 * @file string.h
 * @author Oguzhan KAYNAK
 * @brief This header file declares several functions for manipulating strings.
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
#ifndef _STRING_H
#define _STRING_H 1

#include <sys/cdefs.h>

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/// @brief Compares two memory blocks.
/// @param ptr1 Pointer to the first memory block.
/// @param ptr2 Pointer to the second memory block.
/// @param size Number of bytes to compare.
/// @return Returns an integer less than, equal to, or greater than zero if the first block is found, respectively, to be less than, to match, or be greater than the second block.
int memcmp(const void* ptr1, const void* ptr2, size_t size);

/// @brief Copies bytes from one memory area to another.
/// @param dest Pointer to the destination memory area.
/// @param src Pointer to the source memory area.
/// @param size Number of bytes to copy.
/// @return Returns a pointer to the destination memory area.
void* memcpy(void* dest, const void* src, size_t size);

/// @brief Copies bytes from one memory area to another, handling overlapping memory blocks.
/// @param dest Pointer to the destination memory area.
/// @param src Pointer to the source memory area.
/// @param size Number of bytes to copy.
/// @return Returns a pointer to the destination memory area.
void* memmove(void* dest, const void* src, size_t size);

/// @brief Fills a block of memory with a specified value.
/// @param ptr Pointer to the memory block.
/// @param value Value to be set.
/// @param size Number of bytes to be set to the value.
/// @return Returns a pointer to the memory block.
void* memset(void* ptr, int value, size_t size);

/// @brief Calculates the length of a null-terminated string.
/// @param str Pointer to the null-terminated string.
/// @return Returns the number of characters in the string, excluding the terminating null character.
size_t strlen(const char* str);


#ifdef __cplusplus
}
#endif

#endif
