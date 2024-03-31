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
