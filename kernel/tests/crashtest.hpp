/**
 * @file crashtest.hpp
 * @author Oguzhan KAYNAK
 * @brief A kernel that faults on purpose, to test the fault report.
 *
 * Built with -DATLOS_CRASHTEST=ON (scripts/build.sh crashtest), kernelMain
 * calls run() after boot. scripts/run-crashtest.sh then checks that the
 * serial log shows a page fault at faultAddress and a backtrace.
 */

#pragma once

#include <stdint.h>

namespace CrashTest {

/// @brief Read by run(). Nothing is mapped there: above the identity mapped
/// boot memory, far below the higher half kernel.
static constexpr uintptr_t faultAddress = 0xDEAD0000;

/// @brief Read faultAddress, which must page fault and panic.
[[noreturn]] void run();

} // namespace CrashTest
