/**
 * @file binary.hpp
 * @author Oguzhan KAYNAK ()
 * @brief Utility functions for byte manipulation and bit operations.
 *
 * Copyright © 2024 Oguzhan KAYNAK
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the “Software”), to deal in the Software
 * without  restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit
 * persons to whom the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
 * PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */
#ifndef D08BB42A_B2EF_40C2_AD1B_4AB9EEC1FDF6
#define D08BB42A_B2EF_40C2_AD1B_4AB9EEC1FDF6

#include <stdint.h>

namespace Utils
{

    /**
     * @brief Namespace for byte manipulation and bit operations helper functions.
     */
    namespace Binary
    {

        /**
         * @brief Sets the specified bit flag (to 1).
         *
         * @param value Reference to the variable containing the flag to be set.
         * @param flag The position of the flag to be set (bit number).
         */
        inline void set_flag(uint8_t &value, uint8_t flag)
        {
            value |= (1 << flag);
        }

        /**
         * @brief Resets the specified bit flag (to 0).
         *
         * @param value Reference to the variable containing the flag to be reset.
         * @param flag The position of the flag to be reset (bit number).
         */
        inline void reset_flag(uint8_t &value, uint8_t flag)
        {
            value &= ~(1 << flag);
        }

        /**
         * @brief Toggles the specified bit flag (inverts the bit).
         *
         * @param value Reference to the variable containing the flag to be toggled.
         * @param flag The position of the flag to be toggled (bit number).
         */
        inline void toggle_flag(uint8_t &value, uint8_t flag)
        {
            value ^= (1 << flag);
        }

        /**
         * @brief Checks the state of the specified bit flag.
         *
         * @param value Variable containing the flag.
         * @param flag The position of the flag to be checked (bit number).
         * @return The state of the flag (true if set to 1, false otherwise).
         */
        inline bool is_flag_set(uint8_t value, uint8_t flag)
        {
            return (value & (1 << flag)) != 0;
        }

        /**
         * @brief Applies a mask to a byte value.
         *
         * @param value The byte containing the value to be masked.
         * @param mask The mask to be applied.
         * @return The byte after applying the mask.
         */
        inline uint8_t apply_mask(uint8_t value, uint8_t mask)
        {
            return value & mask;
        }

        /**
         * @brief Combines two bytes into a 16-bit value.
         *
         * @param high The high byte.
         * @param low The low byte.
         * @return The 16-bit value formed by combining the two bytes.
         */
        inline uint16_t combine_bytes(uint8_t high, uint8_t low)
        {
            return (static_cast<uint16_t>(high) << 8) | low;
        }

    } // namespace Binary

} // namespace Utils

#endif /* D08BB42A_B2EF_40C2_AD1B_4AB9EEC1FDF6 */
