// The heap of a user program: set up by atlos_start from the auxiliary vector.
#pragma once

/// The region the kernel mapped (base and size from the auxv, in pages of pageSize).
void atlos_heap_init(unsigned long base, unsigned long size, unsigned long pageSize);
