/**
 * @file kernel.cpp
 * @author Oguzhan KAYNAK
 * @brief Kernel implementation file containing kernel initialization and utility functions.
 *
 * This file contains the implementation of kernel initialization and various utility functions
 * used by the kernel during runtime. It includes functions for hardware initialization,
 * interrupt handling, memory management, and other essential operations.
 * Additionally, it may contain device drivers and other kernel-specific functionalities.
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

#include <stdio.h>
#include <inttypes.h>
#include <kernel/tty.h>

struct myStruct
{
    void print()
    {
        printf("MyStruct::print() called\n");
    }
    myStruct(const char *str)
    {
        printf("MyStruct constructor called:%s\n", str);
    }
};
struct myStruct2
{
    int x;
};
struct myStruct3
{
    int x{4};
};

myStruct glob{"global1"};
myStruct glob2{"global2"};
//  myStruct glob2{};
//  myStruct glob3{};

// myStruct2 glob4{};
// myStruct2 glob5{3};
//   myStruct2 glob6{};

// myStruct3 glob7;

typedef void (*ctor_t)();
extern "C"
{
    extern uint8_t _boottext_start;
    extern uint8_t _boottext_end;
    extern uint8_t _bootrodata_start;
    extern uint8_t _bootrodata_end;
    extern uint8_t _bootdata_start;
    extern uint8_t _bootdata_end;
    extern uint8_t _bootbss_start;
    extern uint8_t _bootbss_end;

    extern uint8_t _text_start;
    extern uint8_t _text_end;
    extern uint8_t _rodata_start;
    extern uint8_t _rodata_end;
    extern uint8_t _data_start;
    extern uint8_t _data_end;
    extern uint8_t _bss_start;
    extern uint8_t _bss_end;

    extern uint8_t __init_array_start;
    extern uint8_t __init_array_end;
    extern uint8_t __fini_array_start;
    extern uint8_t __fini_array_end;

    extern void (*__ctors_start[])(void);
    extern void (*__ctors_end[])(void);

    extern uint8_t kernel_virtual_start;
    extern uint8_t kernel_virtual_end;

    extern uint8_t kernel_physical_start;
    extern uint8_t kernel_physical_end;
}

void print_segment_addresses()
{
    printf("=== Boot Segments ===\n");
    printf("_boottext: %p - %p, size: 0x%X\n", &_boottext_start, &_boottext_end, &_boottext_end - &_boottext_start);
    printf("_bootrodata: %p - %p, size: 0x%X\n", &_bootrodata_start, &_bootrodata_end, &_bootrodata_end - &_bootrodata_start);
    printf("_bootdata: %p - %p, size: 0x%X\n", &_bootdata_start, &_bootdata_end, &_bootdata_end - &_bootdata_start);
    printf("_bootbss: %p - %p, size: 0x%X\n", &_bootbss_start, &_bootbss_end, &_bootbss_end - &_bootbss_start);

    printf("\n=== Kernel Segments (VMA) ===\n");
    printf("_text: %p - %p, size: 0x%X\n", &_text_start, &_text_end, &_text_end - &_text_start);
    printf("_rodata: %p - %p, size: 0x%X\n", &_rodata_start, &_rodata_end, &_rodata_end - &_rodata_start);
    printf("_data: %p - %p, size: 0x%X\n", &_data_start, &_data_end, &_data_end - &_data_start);
    printf("_bss: %p - %p, size: 0x%X\n", &_bss_start, &_bss_end, &_bss_end - &_bss_start);

    printf("__init_array: %p - %p, size: 0x%X\n", &__init_array_start, &__init_array_end, &__init_array_end - &__init_array_start);
    printf("__fini_array: %p - %p, size: 0x%X\n", &__fini_array_start, &__fini_array_end, &__fini_array_end - &__fini_array_start);
    printf("__ctors: %p - %p, size: 0x%zx\n",
           (void *)__ctors_start,
           (void *)__ctors_end,
           (size_t)(__ctors_end - __ctors_start));

    printf("\n=== Kernel Start/End ===\n");
    printf("kernel_virtual_start: %llu, kernel_virtual_end: %llu, size: 0x%X\n", (uint64_t)&kernel_virtual_start, (uint64_t)&kernel_virtual_end, (uint64_t)&kernel_virtual_start - (uint64_t)&kernel_virtual_end);

    printf("\n=== Kernel Physical ===\n");
    printf("kernel_physical_start: %p, kernel_physical_end: %p, size: 0x%X\n", (uint64_t)&kernel_physical_start, (uint64_t)&kernel_physical_end, (uint64_t)&kernel_physical_end - (uint64_t)&kernel_physical_start);
}

/**
 * @brief entry point of atlOS
 *
 */
extern "C" void kernelMain()
{

    terminal_initialize();

    printf("%s\n", "Welcome to atlOS!");

    print_segment_addresses();

    for (auto ctor = __ctors_start; ctor < __ctors_end; ++ctor)
    {
        if ((*ctor == (void (*)(void))-1) || (*ctor == (void (*)(void))0))
            continue;
        (*ctor)();
    }

    myStruct local1{"local1"};

    while (1)
        ;
    return;
}
