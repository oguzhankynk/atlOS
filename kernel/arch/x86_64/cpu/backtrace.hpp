/**
 * @file backtrace.hpp
 * @author Oguzhan KAYNAK
 * @brief Call stack walk through the frame pointer (rbp) chain.
 *
 * The kernel is built with -fno-omit-frame-pointer, so every function starts
 * with "push %rbp; mov %rsp, %rbp": rbp points at the caller's saved rbp, and
 * the return address is right above it. The chain ends at an rbp of 0 (boot.S
 * and new threads start with it).
 */

#pragma once

#include <stdint.h>

namespace Backtrace {

/// @brief At most this many frames are printed.
static constexpr int maxFrames = 16;

/**
 * @brief Collect the return addresses of the frame chain starting at rbp.
 *
 * Stops at a zero or misaligned rbp, a zero return address, or a saved rbp
 * that does not point further up the stack (a broken chain).
 *
 * @return Number of addresses written (at most max).
 */
int collect(uint64_t rbp, uint64_t* addresses, int max);

/**
 * @brief Print rip and then the frames above rbp, one address per line, in a
 * form scripts/symbolize.sh turns into function names.
 */
void print(uint64_t rip, uint64_t rbp);

} // namespace Backtrace
