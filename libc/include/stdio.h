#ifndef _STDIO_H
#define _STDIO_H 1

#include <sys/cdefs.h>

#define EOF (-1)

#ifdef __cplusplus
extern "C" {
#endif

int printf(const char* __restrict, ...);
int putchar(int);
int puts(const char*);

int sprintf(char* buffer, const char* format, ...);

void printMemory(const void* ptr, size_t size);

#ifdef __cplusplus
}
#endif

#endif
