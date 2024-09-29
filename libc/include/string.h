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

/**
 * @brief Compares two memory blocks.
 * 
 * @param ptr1 Pointer to the first memory bl
 * @param ptr2 Pointer to the second memory b
 * @param size Number of bytes to compare.
 * @return int Returns an integer less than, equal to, or greater than zero if the first block is found, respectively, to be less than, to match, or be greater than the second block.
 */
int memcmp(const void* ptr1, const void* ptr2, size_t size);

/**
 * @brief Copies bytes from one memory area to another.
 * 
 * @param dest Pointer to the destination memor
 * @param src Pointer to the source memory area
 * @param size Number of bytes to copy.
 * @return void* Returns a pointer to the destination memory area.
 */
void* memcpy(void* dest, const void* src, size_t size);

/**
 * @brief Copies bytes from one memory area to another, handling overlapping memory blocks.
 * 
 * @param dest Pointer to the destination memory area.
 * @param src Pointer to the source memory area.
 * @param size Number of bytes to copy.
 * @return void* Returns a pointer to the destination memory area.
 */
void* memmove(void* dest, const void* src, size_t size);

/**
 * @brief Fills a block of memory with a specified value.
 * 
 * @param ptr Pointer to the memory block.
 * @param value Value to be set.
 * @param size Number of bytes to be set to the value.
 * @return void* Returns a pointer to the memory block.
 */
void* memset(void* ptr, int value, size_t size);

/**
 * @brief Calculates the length of a null-terminated string.
 * 
 * @param str Pointer to the null-terminated string.
 * @return size_t Returns the number of characters in the string, excluding the terminating null character. 
 */
size_t strlen(const char* str);

/**
 * @brief Compares the C string str1 to the C string str2.
 * 
 * @param str1 C string to be compared.
 * @param str2 C string to be compared.
 * @return int Returns an integral value indicating the relationship between the strings:
 * <0 : the first character that does not match has a lower value in ptr1 than in ptr2
 *  0 : the contents of both strings are equal
 * >0 : the first character that does not match has a greater value in ptr1 than in ptr2
 */
int strcmp(const char *str1, const char *str2);

/**
 * @brief Compares up to num characters of the C string str1 to those of the C string str2.
 * 
 * @param str1 C string to be compared.
 * @param str2 C string to be compared.
 * @param num Maximum number of characters to compare.
 * @return int Returns an integral value indicating the relationship between the strings:
 * <0 : the first character that does not match has a lower value in str1 than in str2
 *  0 : the contents of both strings are equal
 * >0 : the first character that does not match has a greater value in str1 than in str2
 */
int strncmp(const char * str1, const char * str2, size_t num );

/**
 * @brief Returns a pointer to the first occurrence of character in the C string str.
 * 
 * @param str C string.
 * @param character Character to be located. It is passed as its int promotion, but it is internally converted back to char for the comparison.
 * @return char* A pointer to the first occurrence of character in str.
        If the character is not found, the function returns a null pointer.
*/
char* strchr (const char * str, int character);

char* strcpy(char *des, const char *src );

#ifdef __cplusplus
}
#endif

#endif
