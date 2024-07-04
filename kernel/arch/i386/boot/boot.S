# Declare constants for the multiboot header.
.set ALIGN,    1<<0             # align loaded modules on page boundaries
.set MEMINFO,  1<<1             # provide memory map
.set FLAGS,    ALIGN | MEMINFO  # this is the Multiboot 'flag' field
.set MAGIC,    0x1BADB002       # 'magic number' lets bootloader find the header
.set CHECKSUM, -(MAGIC + FLAGS) # checksum of above, to prove we are multiboot

.set KERNEL_STACK_SIZE, 16384  #16KB

# Declare a multiboot header that marks the program as a kernel.
.section .multiboot.data, "aw"
.align 4
.long MAGIC
.long FLAGS
.long CHECKSUM

# Allocate the initial stack.
.section .bootstrap_stack, "aw", @nobits
stack_bottom:
.skip  KERNEL_STACK_SIZE
stack_top:

# Preallocate pages used for paging. Don't hard-code addresses and assume they
# are available, as the bootloader might have loaded its multiboot structures or
# modules there. This lets the bootloader know it must avoid the addresses.
.section .bss, "aw", @nobits
	.align 4096
.global page_directory
page_directory:
	.skip 4096
.global first_page_table
first_page_table:
	.skip 4096
.global second_page_table
second_page_table:
	.skip 4096
# Further page tables may be required if the kernel grows beyond 3 MiB.

# The kernel entry point.
.section .multiboot.text, "a"
.global _start
.type _start, @function
_start:

	# Physical address of first_page_table.
	movl $(first_page_table - 0xC0000000), %edi

	# First address to map is address 0.
	# TODO: Start at the first kernel page instead. Alternatively map the first
	#       1 MiB as it can be generally useful, and there's no need to
	#       specially map the VGA buffer.
	movl $0, %esi
	# Map 1024 pages. 
	movl $1024, %ecx


1:
	# Only map the kernel.
	cmpl $_kernel_start, %esi
	jl 2f
	cmpl $(_kernel_end - 0xC0000000), %esi
	jge 3f

	# Map physical address as "present, writable". Note that this maps
	# .text and .rodata as writable. Mind security and map them as non-writable.
	movl %esi, %edx
	orl $0x003, %edx
	movl %edx, (%edi)



2:
	# Size of page is 4096 bytes.
	addl $4096, %esi
	# Size of entries in first_page_table is 4 bytes.
	addl $4, %edi
	# Loop to the next entry if we haven't finished.
	loop 1b

3:
	movl $(0x000B8000 | 0x003), first_page_table - 0xC0000000 + 1023 * 4
	movl $(first_page_table - 0xC0000000 + 0x003), page_directory - 0xC0000000 + 0
	movl $(first_page_table - 0xC0000000 + 0x003), page_directory - 0xC0000000 + 768 * 4


	movl $(second_page_table - 0xC0000000), %edi

	movl $0, %esi
	# Map 256 pages. 
	movl $256, %ecx


4:
	cmpl $(0x100000), %esi
	jge 5f

	# Map physical address as "present, writable". Note that this maps
	# .text and .rodata as writable. Mind security and map them as non-writable.
	movl %esi, %edx
	orl $0x003, %edx
	movl %edx, (%edi)

	# Size of page is 4096 bytes.
	addl $4096, %esi
	# Size of entries in first_page_table is 4 bytes.
	addl $4, %edi
	# Loop to the next entry if we haven't finished.
	loop 4b


5:
	
	
	
	# Set cr3 to the address of the page_directory.
	movl $(page_directory - 0xC0000000), %ecx
	movl %ecx, %cr3

	# Enable paging and the write-protect bit.
	movl %cr0, %ecx
	orl $0x80010000, %ecx
	movl %ecx, %cr0

	# Jump to higher half with an absolute jump. 
	lea 6f, %ecx
	jmp *%ecx

.section .text

6:

	# At this point, paging is fully set up and enabled.
	movl $(second_page_table + 0x003), page_directory + 0

	# Reload crc3 to force a TLB flush so the changes to take effect.
	movl %cr3, %ecx
	movl %ecx, %cr3

	# Set up the stack.
	mov $stack_top, %esp
	
	# Multiboot headers
	pushl %ebx
	pushl %eax

	call early_kernel

	call _init

	# Enter the high-level kernel.
	call kernel_main

	# Infinite loop if the system has nothing more to do.
	cli
7:	hlt
	jmp 7b

