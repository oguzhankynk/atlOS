/**
 * @file stdarg.h
 * @author okaynak ()
 * @brief Header file containing macros and definitions necessary to support functions with variable numbers of arguments
 * @date 2024-04-17
 * 
 * @copyright Copyright (c) 2024
 * 
 */
#ifndef _STDARG_H
#define _STDARG_H

typedef char* va_list;

#define _INTSIZEOF(n) ((sizeof(n) + sizeof(int) - 1) & ~(sizeof(int) - 1))

#define va_start(ap, last_arg) (ap = (va_list)&last_arg + _INTSIZEOF(last_arg))
#define va_arg(ap, type) (*(type*)((ap += _INTSIZEOF(type)) - _INTSIZEOF(type)))
#define va_end(ap) (ap = (va_list)0)

#endif /* _STDARG_H */