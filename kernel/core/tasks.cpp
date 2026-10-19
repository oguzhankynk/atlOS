#include <sched/scheduler.hpp>
#include <stdio.h>

// Demo threads: each prints its letter on the console and yields

void threadTaskA(Threading::Scheduler& scheduler)
{
    while (true) {
        printf("A\n");
        for (volatile int i = 0; i < 5000000; i++)
            ;
        scheduler.yield();
    }
}

void threadTaskB(Threading::Scheduler& scheduler)
{
    while (true) {
        printf("B\n");
        for (volatile int i = 0; i < 5000000; i++)
            ;
        scheduler.yield();
    }
}
