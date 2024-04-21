/**
 * @file MemoryArrayMappedAddress.hpp
 * @author okaynak ()
 * @brief Reading the Memory Device (Type 17) structure field in SMBIOS
 * @date 2024-04-03
 * 
 * @copyright Copyright (c) 2024
 * 
 */
#ifndef C6BC6169_1E64_4073_B39E_B77AE44C5066
#define C6BC6169_1E64_4073_B39E_B77AE44C5066

#include <stdint.h>
#include <stdio.h>
#include "kernel/private/BIOS/BIOSStructureCommon.hpp"

/// @brief Class to represent the Memory Array Mapped Address table (Type 19).
class MemoryArrayMappedAddress : public BIOSStructureCommon
{
public:
    /// @brief Default constructor.
    MemoryArrayMappedAddress() = default;
    
    /// @brief Constructor that takes a pointer to the start of the structure.
    /// @param p_start Pointer to the start of the structure.
    MemoryArrayMappedAddress(void *p_start) 
    {
        Scan(p_start);
    }

    /// @brief Scans the Memory Array Mapped Address table.
    /// @param p_start Pointer to the start of the structure.
    /// @return Pointer to the end of the structure.
    void *Scan(void *p_start) override
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

    /// @brief Gets the Starting Address value.
    /// @return The Starting Address value.
    uint32_t getStartingAddress() const { return *getStartingAddressAddr(); }

    /// @brief Gets the Ending Address value.
    /// @return The Ending Address value.
    uint32_t getEndingAddress() const { return *getEndingAddressAddr(); }

    /// @brief Gets the Memory Array Handle value.
    /// @return The Memory Array Handle value.
    uint16_t getMemoryArrayHandle() const { return *getMemoryArrayHandleAddr(); }

    /// @brief Gets the Partition Width value.
    /// @return The Partition Width value.
    uint8_t getPartitionWidth() const { return *getPartitionWidthAddr(); }

    /// @brief Gets the Extended Starting Address value.
    /// @return The Extended Starting Address value.
    uint64_t getExtendedStartingAddress() const { return *getExtendedStartingAddressAddr(); }

    /// @brief Gets the Extended Ending Address value.
    /// @return The Extended Ending Address value.
    uint64_t getExtendedEndingAddress() const { return *getExtendedEndingAddressAddr(); }

    // Print function to display all values
    void print() const 
    {
        printf("Type: %u\n", getType());
        printf("Length: %u\n", getLength());
        printf("Handle: %u\n", getHandle());
        printf("Starting Address: %u\n", getStartingAddress());
        printf("Ending Address: %u\n", getEndingAddress());
        printf("Memory Array Handle: %u\n", getMemoryArrayHandle());
        printf("Partition Width: %u\n", getPartitionWidth());
        printf("Extended Starting Address: %llu\n", getExtendedStartingAddress());
        printf("Extended Ending Address: %llu\n", getExtendedEndingAddress());
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

    /// @brief Gets the address of the Starting Address field.
    /// @return Pointer to the Starting Address field.
    uint32_t* getStartingAddressAddr() const { return reinterpret_cast<uint32_t*>(reinterpret_cast<uint8_t*>(m_pStart) + 0x04); }

    /// @brief Gets the address of the Ending Address field.
    /// @return Pointer to the Ending Address field.
    uint32_t* getEndingAddressAddr() const { return reinterpret_cast<uint32_t*>(reinterpret_cast<uint8_t*>(m_pStart) + 0x08); }

    /// @brief Gets the address of the Memory Array Handle field.
    /// @return Pointer to the Memory Array Handle field.
    uint16_t* getMemoryArrayHandleAddr() const { return reinterpret_cast<uint16_t*>(reinterpret_cast<uint8_t*>(m_pStart) + 0x0C); }

    /// @brief Gets the address of the Partition Width field.
    /// @return Pointer to the Partition Width field.
    uint8_t* getPartitionWidthAddr() const { return reinterpret_cast<uint8_t*>(reinterpret_cast<uint8_t*>(m_pStart) + 0x0E); }

    /// @brief Gets the address of the Extended Starting Address field.
    /// @return Pointer to the Extended Starting Address field.
    uint64_t* getExtendedStartingAddressAddr() const { return reinterpret_cast<uint64_t*>(reinterpret_cast<uint8_t*>(m_pStart) + 0x0F); }

    /// @brief Gets the address of the Extended Ending Address field.
    /// @return Pointer to the Extended Ending Address field.
    uint64_t* getExtendedEndingAddressAddr() const { return reinterpret_cast<uint64_t*>(reinterpret_cast<uint8_t*>(m_pStart) + 0x17); }

};

#endif /* C6BC6169_1E64_4073_B39E_B77AE44C5066 */
