/**
 * @file kernel.c
 * @author okaynak ()
 * @brief Kernel implementation file containing kernel initialization and utility functions.
 *
 * This file contains the implementation of kernel initialization and various utility functions
 * used by the kernel during runtime. It includes functions for hardware initialization,
 * interrupt handling, memory management, and other essential operations.
 * Additionally, it may contain device drivers, system calls, and other kernel-specific functionalities.
 * @date 2024-03-31
 * 
 * @copyright Copyright (c) 2024
 * 
 */
#include <stdio.h>
#include <string.h>

#include <kernel/public/tty.h>

#include <kernel/public/cpu.h>
#include <kernel/public/BIOS/BIOS.h>

extern "C" void kernel_main(void);


/**
 * @brief firs point of kaynakOS
 * 
 */
void kernel_main(void) 
{
	terminal_initialize();

	printf("HELLO WORLD FROM kaynakOS\n");

	BIOSInit();

	while(1);
}
