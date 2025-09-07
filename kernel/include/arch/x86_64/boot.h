/**
 * @file boot.h
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

#include <arch/x86_64/gdt.inc>
#include <arch/x86_64/mmu.inc>
#include <arch/x86_64/msr.inc>
#include <arch/x86_64/multiboot2.inc>

#define KERNEL_BOOT_STACK_SIZE 0x4000
#define KERNEL_BOOT_STACK_ALIGNMENT 0x1000

#define KERNEL_PHYSICAL_START 0x0000000000400000
#define KERNEL_VIRTUAL_LOAD_OFFSET 0xFFFFFFFF80000000
#define KERNEL_VIRTUAL_START (KERNEL_PHYSICAL_START + KERNEL_VIRTUAL_LOAD_OFFSET)

#define KERNEL_GDT_ENTRY 1

#define CONTROL_REGISTER0_PROTECTED_MODE_ENABLED (1 << 0)
#define CONTROL_REGISTER0_EXTENSION_TYPE (1 << 4)
#define CONTROL_REGISTER0_PAGE (1 << 31)
#define KERNEL_CR0 (CONTROL_REGISTER0_PAGE | CONTROL_REGISTER0_PROTECTED_MODE_ENABLED | CONTROL_REGISTER0_EXTENSION_TYPE)

#define CONTROL_REGISTER4_PAGE_SIZE_EXTENSION (1 << 4)
#define CONTROL_REGISTER4_PHYSICAL_ADDRESS_EXTENSION (1 << 5)
#define KERNEL_CR4 (CONTROL_REGISTER4_PHYSICAL_ADDRESS_EXTENSION)
