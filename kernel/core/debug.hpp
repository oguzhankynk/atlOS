/**
 * @file debug.hpp
 * @author Oguzhan KAYNAK
 * @brief ATLOS_BREAKPOINT(): stop for a moment and show the state.
 *
 * Executes int3. The breakpoint handler prints the registers and a backtrace
 * on the console, then the code goes on after the breakpoint. Put it where
 * you want to know "did it get here, and with what".
 */

#pragma once

#define ATLOS_BREAKPOINT() __asm__ volatile("int3" :: \
                                                : "memory")
