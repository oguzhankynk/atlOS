/**
 * @file parser.hpp
 * @author Oguzhan KAYNAK
 * @brief Splitting a command line into words, and numbers in the words.
 * Pure functions, tested without a console.
 */

#pragma once

#include <stdint.h>

namespace Shell {

/**
 * @brief Split line in place into words: spaces and tabs separate them,
 * "double quotes" keep spaces in a word (the quotes are removed).
 *
 * @return The number of words (0 for an empty line), -1 for a missing
 * closing quote or more than maxWords words.
 */
int splitWords(char* line, char* words[], int maxWords);

/**
 * @brief Parse "0x1f" (hexadecimal) or "31" (decimal) into value.
 * @return false for an empty string, other characters or an overflow.
 */
bool parseNumber(const char* text, uint64_t& value);

} // namespace Shell
