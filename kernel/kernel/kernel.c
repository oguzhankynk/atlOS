#include <stdio.h>
#include <string.h>

#include <kernel/tty.h>

#include <kernel/cpu.h>
#include <kernel/BIOS.h>



void kernel_main(void) 
{
	terminal_initialize();

	printf("HELLO WORLD FROM kaynakOS\n");

	//CPU_Info cpuInfo;
	//InitiliazeCPUInfo(&cpuInfo);

	//printCPUInfo(&cpuInfo);
	SMBIOSEntryPoint BIOS_handle;

	SMBIOSInit(&BIOS_handle);

	
}
