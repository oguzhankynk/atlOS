/**
 * @file tty.h
 * @author Oguzhan KAYNAK
 * @brief Header file defining functions and structures related to terminal devices (TTY).
 *  TTY represents a terminal device that interacts with the user.
 *  This header file defines functions that perform terminal input/output operations.
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
#ifndef _KERNEL_TTY_H
#define _KERNEL_TTY_H

#include <stddef.h>

#if defined(__cplusplus)
extern "C" {
#endif

/// @brief Initializes the terminal.
/// @param
void terminal_initialize(void);

/// @brief Writes a single character to the terminal.
/// @param c The character to write.
void terminal_putchar(char c);

/// @brief Writes a buffer of characters to the terminal.
/// @param data Pointer to the buffer containing the characters to write.
/// @param size Number of characters to write from the buffer.
void terminal_write(const char* data, size_t size);

/// @brief Writes a null-terminated string to the terminal.
/// @param data Pointer to the null-terminated string to write.
void terminal_writestring(const char* data);

#if defined(__cplusplus)
} /* extern "C" */
#endif

#endif
