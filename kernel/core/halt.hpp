/**
 * @file halt.hpp
 * @author Oguzhan KAYNAK
 * @brief Stopping the machine for good.
 */

#pragma once

#include <stdint.h>

namespace Core {

/// @brief Why the kernel stops (the value written to the debug exit port).
enum class ExitStatus : uint8_t {
    Success = 0, ///< e.g. every self test passed
    Failure = 1, ///< e.g. a self test failed
    Panic = 2, ///< kernelPanic or an unhandled CPU exception
};

/// @brief Disable interrupts and halt the CPU forever.
[[noreturn]] void halt();

/**
 * @brief Report status, then halt.
 *
 * Built with ATLOS_DEBUG_EXIT_PORT (the self test presets), the status is first
 * written to that I/O port: a test machine can have a device there that ends
 * the run with it, so a test script gets the result without a timeout. Without
 * the option nothing is written, as on a real machine.
 */
[[noreturn]] void haltWithStatus(ExitStatus status);

} // namespace Core
