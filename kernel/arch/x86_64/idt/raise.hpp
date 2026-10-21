/**
 * @file raise.hpp
 * @author Oguzhan KAYNAK
 * @brief Raising any interrupt vector in software ("int n"), for the shell.
 */

#pragma once

#include <stdint.h>

namespace IDT {

/**
 * @brief Execute "int vector" in ring 0.
 *
 * The CPU pushes an error code only when it raises an exception itself, never
 * for "int n"; the vectors whose stub expects one (8, 10..14, 17, 21) are
 * entered by building the frame by hand, with a 0 error code, and jumping to
 * the stub. An exception vector
 * then reports and panics like a real fault; an IRQ vector runs its handler
 * as if the device had interrupted.
 */
void raise(uint8_t vector);

/// @brief Whether the CPU pushes an error code for this exception vector.
constexpr bool hasErrorCode(uint32_t vector)
{
    return vector == 8 || (vector >= 10 && vector <= 14) || vector == 17 || vector == 21;
}

} // namespace IDT
