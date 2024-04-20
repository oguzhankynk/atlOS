/**
 * @file tty.h
 * @author okaynak ()
 * @brief Header file defining functions and structures related to terminal devices (TTY).
 *  TTY represents a terminal device that interacts with the user.
 *  This header file defines functions that perform terminal input/output operations.
 * @date 2024-03-31
 * 
 * @copyright Copyright (c) 2024
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
