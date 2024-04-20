#include <stdio.h>
#include <stddef.h>

#if defined(__is_libk)
extern "C" void terminal_write(const char* data, size_t size);
#endif

int putchar(int ic) {
#if defined(__is_libk)
	
	char c = (char) ic;
	terminal_write(&c, sizeof(c));
#else
	// TODO: Implement stdio and the write system call.
#endif
	return ic;
}
