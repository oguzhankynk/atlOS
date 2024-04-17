/**
 * @file SystemBootInformation.hpp
 * @author okaynak ()
 * @brief Reading the System Boot Information (Type 32)field in SMBIOS
 * @date 2024-04-03
 * 
 * @copyright Copyright (c) 2024
 * 
 */
#ifndef E600C230_FFAE_4392_A5C8_6582F0F6DA30
#define E600C230_FFAE_4392_A5C8_6582F0F6DA30

#include <stdint.h>
#include <stdio.h>
#include "kernel/private/BIOS/BIOSStructureCommon.hpp"

/// @brief Class to represent the System Boot Information table (Type 32).
class SystemBootInformation : public BIOSStructureCommon
{
public:
    /// @brief Default constructor.
    SystemBootInformation() = default;
    
    /// @brief Constructor that takes a pointer to the start of the structure.
    /// @param p_start Pointer to the start of the structure.
    SystemBootInformation(void *p_start) 
    {
        Scan(p_start);
    }

    /// @brief Scans the System Boot Information table.
    /// @param p_start Pointer to the start of the structure.
    /// @return Pointer to the end of the structure.
    void *Scan(void *p_start) 
    {
        m_pStart = p_start;

        int end = 1;
        uint8_t *ptr = (uint8_t *)m_pStart + getLength();

        for (; ptr[end - 1] != '\0' || ptr[end] != '\0'; end++);

        m_pEnd = ptr + end + 1;

        return m_pEnd;
    }

    // Getter methods for accessing the values of different fields.

    /// @brief Gets the Type value.
    /// @return The Type value.
    uint8_t getType() const { return *getTypeAddr(); }

    /// @brief Gets the Length value.
    /// @return The Length value.
    uint8_t getLength() const { return *getLengthAddr(); }

    /// @brief Gets the Handle value.
    /// @return The Handle value.
    uint16_t getHandle() const { return *getHandleAddr(); }

    /// @brief Gets the Boot Status value.
    /// @return The Boot Status value.
    uint8_t getBootStatus() const { return *getBootStatusAddr(); }

    // Print function to display all values
    void print() const 
    {
        printf("Type: %u\n", getType());
        printf("Length: %u\n", getLength());
        printf("Handle: %u\n", getHandle());
        printf("Boot Status: %u\n", getBootStatus());
    }

private:
    void *m_pStart{}; ///< Start address of the structure.
    void *m_pEnd{};   ///< End address of the structure.

    // Private methods to get the addresses of different fields.

    /// @brief Gets the address of the Type field.
    /// @return Pointer to the Type field.
    uint8_t* getTypeAddr() const { return reinterpret_cast<uint8_t*>(m_pStart) + 0x00; }

    /// @brief Gets the address of the Length field.
    /// @return Pointer to the Length field.
    uint8_t* getLengthAddr() const { return reinterpret_cast<uint8_t*>(m_pStart) + 0x01; }

    /// @brief Gets the address of the Handle field.
    /// @return Pointer to the Handle field.
    uint16_t* getHandleAddr() const { return reinterpret_cast<uint16_t*>(reinterpret_cast<uint8_t*>(m_pStart) + 0x02); }

    /// @brief Gets the address of the Boot Status field.
    /// @return Pointer to the Boot Status field.
    uint8_t* getBootStatusAddr() const { return reinterpret_cast<uint8_t*>(reinterpret_cast<uint8_t*>(m_pStart) + 0x0A); }

};


#endif /* E600C230_FFAE_4392_A5C8_6582F0F6DA30 */
