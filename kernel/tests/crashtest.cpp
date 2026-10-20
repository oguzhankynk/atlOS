/**
 * @file crashtest.cpp
 * @author Oguzhan KAYNAK
 * @brief A kernel that faults on purpose, to test the fault report.
 */

#include <tests/crashtest.hpp>

#include <stdio.h>

#include <core/halt.hpp>

namespace CrashTest {

// A function of its own, so the backtrace has a frame to show above the fault
__attribute__((noinline)) static uint64_t readUnmapped()
{
    return *reinterpret_cast<volatile uint64_t*>(faultAddress);
}

void run()
{
    printf("[CRASHTEST] reading unmapped address 0x%lx\n", faultAddress);
    uint64_t value = readUnmapped();

    // Only reached when the read did not fault
    printf("[CRASHTEST] FAIL: read 0x%lx without a page fault\n", value);
    Core::haltWithStatus(Core::ExitStatus::Failure);
}

} // namespace CrashTest
