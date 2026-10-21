/* A breakpoint in ring 3: the kernel shows the registers, the program goes on. */
#include "atlos.h"

int main(void)
{
    print("before the breakpoint\n");
    breakpoint();
    print("after the breakpoint\n");
    return 0;
}
