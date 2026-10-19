/**
 * @file structure_common.hpp
 * @author Oguzhan KAYNAK
 * @brief Base class that sub-BIOS classes will use
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
#pragma once

/// @brief Class representing common functionality for handling BIOS structures.
class BIOSStructureCommon {
public:
    /// @brief Extract the strings that at end of BIOS structure definations
    /// @param strStart String starting address of the BIOS structure
    /// @param strEnd End address of the BIOS structure
    /// @param addresses array of strings that fill with extracted strings
    /// @param maxStrings length of 'addresses' array
    void extractStringAddress(const char* strStart, const char* strEnd, const char* addresses[], unsigned int maxStrings)
    {
        unsigned int count = 0;
        while (*strStart != '\0' && count < maxStrings && strStart <= strEnd) {
            addresses[count++] = strStart;
            strStart += strlen(strStart) + 1; // Bir sonraki dizeye geç
        }
    }

    virtual void* scan(void* pStart) = 0;

    virtual void print() const = 0;
};
