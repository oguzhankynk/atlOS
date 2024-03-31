/**
 * @file BIOSStructureCommon.hpp
 * @author okaynak ()
 * @brief Base class that sub-BIOS classes will use
 * @date 2024-03-31
 * 
 * @copyright Copyright (c) 2024
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
};

#endif /* C4ED04D9_0FCD_4B1F_9597_24FD51FBB80F */
