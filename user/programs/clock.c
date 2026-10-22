/*
 * The clock system calls: uptime (milliseconds since boot), sleep and yield.
 * Exits with 0 if the sleep took as long as asked.
 */
#include "atlos.h"

int main(void)
{
    unsigned long start = uptime();
    print("up ");
    print_number(start);
    print(" ms, sleeping 200 ms\n");

    sleep(200); // the other threads run meanwhile, the CPU halts if none can
    yield();
    unsigned long slept = uptime() - start;

    print("woke up after ");
    print_number(slept);
    print(" ms\n");
    return slept >= 200 ? 0 : 1;
}
