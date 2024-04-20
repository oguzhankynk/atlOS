/**
 * @file stdlib.h
 * @author okaynak ()
 * @brief This header file declares several functions for performing general utilities, memory management, and other tasks.
 * @date 2024-03-31
 * 
 * @copyright Copyright (c) 2024
 * 
 */
#ifndef _STDLIB_H
#define _STDLIB_H 1

#include <sys/cdefs.h>

#ifdef __cplusplus
extern "C" {
#endif

/// @brief abort the program
__attribute__((__noreturn__)) void abort(void);


#ifdef __cplusplus
}
#endif

#endif
