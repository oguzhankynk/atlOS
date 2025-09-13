/**
 * @file boot.inc
 * @author Oguzhan KAYNAK
 * @brief
 *
 * Copyright © 2024 Oguzhan KAYNAK
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the “Software”), to deal in the Software *without  restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit *persons to whom the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR *PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR *OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include <arch/x86_64/boot/multiboot2.inc>
#include <arch/x86_64/MMU/mmu.inc>
#include <arch/x86_64/cpu/cpu.hpp>

#ifndef KERNEL_PHYSICAL_START
#error "KERNEL_PHYSICAL_START is not defined! Please define it in CMake or before including this file."
#endif

#ifndef KERNEL_VIRTUAL_LOAD_OFFSET
#error "KERNEL_VIRTUAL_LOAD_OFFSET is not defined! Please define it in CMake or before including this file."
#endif

#define KERNEL_BOOT_STACK_SIZE 0x4000
#define KERNEL_BOOT_STACK_ALIGNMENT 0x1000

#define KERNEL_VIRTUAL_START (KERNEL_PHYSICAL_START + KERNEL_VIRTUAL_LOAD_OFFSET)


