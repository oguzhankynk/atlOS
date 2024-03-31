/**
 * @file string.h
 * @author okaynak ()
 * @brief This header file declares several functions for manipulating strings.
 * @date 2024-03-31
 * 
 * @copyright Copyright (c) 2024
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
