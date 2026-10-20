/**
 * @file backtrace.cpp
 * @author Oguzhan KAYNAK
 * @brief Call stack walk through the frame pointer (rbp) chain.
 */

#include <arch/x86_64/cpu/backtrace.hpp>

#include <stdio.h>

namespace Backtrace {

int collect(uint64_t rbp, uint64_t* addresses, int max)
{
    int count = 0;

    while (count < max && rbp && !(rbp % sizeof(uint64_t))) {
        // frame[0] = the caller's rbp, frame[1] = the return address into the caller
        const uint64_t* frame = reinterpret_cast<const uint64_t*>(rbp);
        if (!frame[1])
            break;
        addresses[count++] = frame[1];

        // The stack grows down: a caller's frame is always at a higher address
        if (frame[0] <= rbp)
            break;
        rbp = frame[0];
    }

    return count;
}

void print(uint64_t rip, uint64_t rbp)
{
    uint64_t addresses[maxFrames];
    int count = collect(rbp, addresses, maxFrames);

    printf("Backtrace (scripts/symbolize.sh shows the functions):\n");
    printf("  #0 0x%016lx\n", rip);
    for (int i = 0; i < count; i++)
        printf("  #%d 0x%016lx\n", i + 1, addresses[i]);
}

} // namespace Backtrace
