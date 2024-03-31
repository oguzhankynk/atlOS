/**
 * @file SMBIOS.hpp
 * @author okaynak ()
 * @brief SMBIOS fields and CIM MOF properties
 * @version 0.1
 * @date 2024-03-31
 * 
 * @copyright Copyright (c) 2024
 * 
 */

#ifndef CC04DEBE_B0F3_4321_AA29_F8861E1DEABD
#define CC04DEBE_B0F3_4321_AA29_F8861E1DEABD

#include <kernel/private/BIOS/SMBIOS/BIOSInformation.hpp>
#include <kernel/private/BIOS/SMBIOS/SystemInformation.hpp>

/// @brief Structure representing the SMBIOS Entry Point.
struct SMBIOSEntryPoint
{
    char signature[4];               ///< Signature identifying the SMBIOS Entry Point.
    uint8_t checksum;                ///< Checksum of the Entry Point structure.
    uint8_t length;                  ///< Length of the Entry Point structure.
    uint8_t majorVersion;            ///< Major version of the SMBIOS specification.
    uint8_t minorVersion;            ///< Minor version of the SMBIOS specification.
    uint16_t maximumStructureSize;   ///< Maximum size of the SMBIOS structure.
    uint8_t entryPointRevision;      ///< Revision of the Entry Point structure.
    char formattedArea[5];           ///< Formatted area.
    char intermediateAnchor[5];      ///< Intermediate anchor string.
    uint8_t intermediateChecksum;   ///< Checksum of the intermediate structure.
    uint16_t structureTableLength;   ///< Length of the structure table.
    uint32_t structureTableAddress;  ///< Address of the structure table.
    uint16_t numberOfStructures;     ///< Number of structures in the structure table.
    uint8_t bcdRevision;             ///< BCD revision.
};

/// @brief Class for handling SMBIOS data.
class SMBIOS
{
public:
    /// @brief Default constructor.
    SMBIOS();

    /// @brief Prints the SMBIOS entry point information.
    void printEntryPoint();

    /// @brief Prints the BIOS information.
    void printBIOSInformation();

    /// @brief Prints the system information.
    void printSystemInformation();

private:
    SMBIOSEntryPoint m_SMBIOSEntryPoint; ///< SMBIOS entry point structure.
    BIOSInformation m_BIOSInformation;   ///< BIOS information structure.
    SystemInformation m_SystemInformation; ///< System information structure.
};

#endif /* CC04DEBE_B0F3_4321_AA29_F8861E1DEABD */
