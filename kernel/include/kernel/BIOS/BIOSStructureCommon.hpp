/**
 * @file BIOSStructureCommon.hpp
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
#ifndef C4ED04D9_0FCD_4B1F_9597_24FD51FBB80F
#define C4ED04D9_0FCD_4B1F_9597_24FD51FBB80F

/// @brief Class representing common functionality for handling BIOS structures.
class BIOSStructureCommon
{
public:
    /// @brief Extract the strings that at end of BIOS structure definations
    /// @param str_start String starting address of the BIOS structure
    /// @param str_end End address of the BIOS structure
    /// @param addresses array of strings that fill with extracted strings
    /// @param MAX_STRINGS length of 'addresses' array
    void extractStringAddress(const char *str_start, const char *str_end, const char *addresses[], unsigned int MAX_STRINGS)
    {
        unsigned int count = 0;
        while (*str_start != '\0' && count < MAX_STRINGS && str_start <= str_end)
        {
            addresses[count++] = str_start;
            str_start += strlen(str_start) + 1; // Bir sonraki dizeye geç
        }
    }

    virtual void *Scan(void *p_start) = 0;

    virtual void print() const = 0;
};

#endif /* C4ED04D9_0FCD_4B1F_9597_24FD51FBB80F */
