/*
 * The example of user/README.md: counts to 5, then exits with the sum.
 * Shows a loop, a helper function, a global in .bss and the exit code.
 */
#include "atlos.h"

static long s_sum; // .bss: starts at 0

// No printf: turn a number into text by hand
static void printNumber(long value)
{
    char text[24];
    int length = 0;
    do {
        text[length++] = (char)('0' + value % 10);
        value /= 10;
    } while (value);

    char reversed[24];
    for (int i = 0; i < length; i++)
        reversed[i] = text[length - 1 - i];
    write(1, reversed, length);
}

int main(void)
{
    for (long i = 1; i <= 5; i++) {
        printNumber(i);
        print(i < 5 ? " " : "\n");
        s_sum += i;
    }
    return s_sum; // 15: shown as "exited with 15"
}
