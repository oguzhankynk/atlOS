/**
 * @file stdio.h
 * @author okaynak ()
 * @brief This header file declares several functions for performing input and output.
 * @date 2024-03-31
 * 
 * @copyright Copyright (c) 2024
 * 
 */
#ifndef _STDIO_H
#define _STDIO_H 1

#include <sys/cdefs.h>

#define EOF (-1)

#ifdef __cplusplus
extern "C" {
#endif

/// @brief Prints formatted output to stdout.
/// @param format A pointer to a null-terminated string that contains the format string.
/// @param ... Additional arguments that are inserted into the format string.
/// @return On success, returns the number of characters printed (excluding the null byte used to end output to strings); On failure, a negative number is returned.
int printf(const char* __restrict format, ...);

/// @brief Writes a character (an unsigned char) specified by the argument to stdout.
/// @param character The character to be written.
/// @return On success, returns the character written as an unsigned char cast to an int; On failure, EOF is returned.
int putchar(int character);

/// @brief Writes the string pointed to by str to stdout followed by a newline character '\n'.
/// @param str The string to be written.
/// @return On success, a non-negative value is returned; On failure, EOF is returned.
int puts(const char* str);

/// @brief Writes formatted data to a string.
/// @param buffer Pointer to the buffer where the resulting C string is stored.
/// @param format Pointer to a null-terminated string that contains the format string.
/// @param ... Additional arguments that are inserted into the format string.
/// @return The number of characters written into the array, not including the null-terminating character.
int sprintf(char* buffer, const char* format, ...);

/// @brief Prints the memory content starting from the specified pointer.
/// @param ptr Pointer to the memory block to be printed.
/// @param size Number of bytes to print.
void printMemory(const void* ptr, unsigned int size);


#ifdef __cplusplus
}
#endif

#endif
