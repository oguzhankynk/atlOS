/* The first atlOS user program: prints from ring 3 and exits with 0. */
#include "atlos.h"

static int s_calls; // in .bss: zeroed by the kernel
static char s_message[] = "Hello from user mode!\n"; // in .data: a writable page

int main(void)
{
    s_message[0] = 'H'; // writing to its own data page is allowed
    s_calls++;
    print(s_message);

    // The code segment is at userBase (0x0000008000000000)
    char line[] = "running at 0x0000008000xxxxxx\n";
    unsigned long address = (unsigned long)&main;
    const char* hex = "0123456789abcdef";
    for (int i = 0; i < 6; i++)
        line[28 - i] = hex[(address >> (4 * i)) & 0xf];
    print(line);

    return s_calls == 1 ? 0 : 1;
}
