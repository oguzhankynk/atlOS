/*
 * Hands the kernel pointers it must refuse: kernel memory, the kernel only
 * low memory, an unmapped user address, a wrong file descriptor, an unknown
 * system call. Exits with 0 when every one is refused as expected.
 */
#include "atlos.h"

int main(void)
{
    int wrong = 0;
    wrong += write(1, (const void*)0xFFFFFFFF80000000UL, 8) != -EFAULT; // the kernel image
    wrong += write(1, (const void*)0xB8000, 8) != -EFAULT; // identity map: kernel only
    wrong += write(1, (const void*)0x0000010000000000UL, 8) != -EFAULT; // user range, not mapped
    wrong += write(1, (const void*)0x00007FFFFFFFFFF0UL, 32) != -EFAULT; // runs past the user range
    wrong += write(5, "x", 1) != -EBADF;
    wrong += atlos_syscall(99, 0, 0, 0) != -ENOSYS;
    wrong += write(1, "", 0) != 0;

    print(wrong ? "badptr: something was accepted\n" : "badptr: every bad pointer refused\n");
    return wrong;
}
