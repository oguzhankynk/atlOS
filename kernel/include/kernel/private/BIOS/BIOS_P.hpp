/**
 * @file BIOS_P.hpp
 * @author okaynak ()
 * @brief Inner class of which handles BIOS
 * @date 2024-03-31
 * 
 * @copyright Copyright (c) 2024
 * 
 */

#ifndef C1A89A8C_95D9_4708_8742_B82CD9D74F7A
#define C1A89A8C_95D9_4708_8742_B82CD9D74F7A

#include <kernel/private/BIOS/SMBIOS/SMBIOS.hpp>

/// @brief Class representing BIOS information.
class BIOS : public SMBIOS
{
public:
    /// @brief Default constructor.
    BIOS() = default;
};


#endif /* C1A89A8C_95D9_4708_8742_B82CD9D74F7A */
