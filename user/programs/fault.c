/*
 * Ends with a page fault: writes to address 0. The first megabytes are
 * mapped in every address space, but for the kernel only (no user bit), so
 * this is a protection violation. The kernel reports it and ends the
 * program with exit code 128 + 14; the kernel itself goes on.
 */
#include "atlos.h"

int main(void)
{
    print("about to write to address 0\n");
    *(volatile int*)0 = 1;
    print("not reached\n");
    return 0;
}
