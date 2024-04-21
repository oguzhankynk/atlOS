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
#include <kernel/private/BIOS/SMBIOS/BaseBoardInformation.hpp>
#include <kernel/private/BIOS/SMBIOS/SystemEnclosureChassis.hpp>
#include <kernel/private/BIOS/SMBIOS/ProcessorInformation.hpp>
#include <kernel/private/BIOS/SMBIOS/PhysicalMemoryArray.hpp>
#include <kernel/private/BIOS/SMBIOS/MemoryDevice.hpp>
#include <kernel/private/BIOS/SMBIOS/MemoryArrayMappedAddress.hpp>
#include <kernel/private/BIOS/SMBIOS/SystemBootInformation.hpp>
#include <kernel/private/BIOS/SMBIOS/EndOfTable.hpp>

/// @brief Structure representing the SMBIOS Entry Point.
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
    void printEntryPoint() const;

    /// @brief Prints the BIOS information.
    void printBIOSInformation() const;

    /// @brief Prints the system information.
    void printSystemInformation() const;

    /// @brief Prints the baseboard information.
    void printBaseBoardInformation() const;

    /// @brief Prints the baseboard information.
    void printSystemEnclosureChassis() const;

    /// @brief Prints the Processor Information.
    void printProcessorInformation() const;

    /// @brief Prints the PhysicalMemoryArray Information.
    void printPhysicalMemoryArray() const ;

    /// @brief Prints the PhysicalMemoryArray Information.
    void printMemoryDevice() const ;

    /// @brief Prints the MemoryArrayMappedAddress.
    void printMemoryArrayMappedAddress() const;

    /// @brief Prints the SystemBootInformation.
    void printSystemBootInformation() const;

private:
    SMBIOSEntryPoint m_SMBIOSEntryPoint; ///< SMBIOS entry point structure.
    BIOSInformation m_BIOSInformation;   ///< BIOS information structure.
    SystemInformation m_SystemInformation; ///< System information structure.
    BaseBoardInformation m_BaseBoardInformation; ///< BaseBoardInformation structure.
    SystemEnclosureChassis m_SystemEnclosureChassis; ///< SystemEnclosureChassis structure.
    ProcessorInformation m_ProcessorInformation; ///< ProcessorInformation structure.
    PhysicalMemoryArray m_PhysicalMemoryArray; ///< PhysicalMemoryArray structure.
    MemoryDevice m_MemoryDevice; ///< MemoryDevice structure.
    MemoryArrayMappedAddress m_MemoryArrayMappedAddress; ///< MemoryDevice structure.
    SystemBootInformation m_SystemBootInformation; ///< System Boot Information
    EndOfTable m_EndOfTable; ///< System Boot Information
};

#endif /* CC04DEBE_B0F3_4321_AA29_F8861E1DEABD */
