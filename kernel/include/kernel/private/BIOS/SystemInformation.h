#ifndef B753BF1E_92A3_45A6_ABED_F40B5BE6B059
#define B753BF1E_92A3_45A6_ABED_F40B5BE6B059

#include <stdint.h>

typedef struct {
    uint8_t type;                 // System Information indicator (SMBIOS Type 1)
    uint8_t length;               // Length field (Length of the structure)
    uint16_t handle;              // Handle field (Unique handle for the structure)
    uint8_t manufacturer;         // Number of null-terminated string
    uint8_t productName;          // Number of null-terminated string
    uint8_t version;              // Number of null-terminated string
    uint8_t serialNumber;         // Number of null-terminated string
    uint8_t uuid[16];             // Universal unique ID number
    uint8_t wakeUpType;           // Identifies the event that caused the system to power up.
    uint8_t skuNumber;            // Number of null-terminated string
    uint8_t family;               // Number of null-terminated string
} SMBIOSSystemInfo;

void* SystemInformation(const void* startAddr);
void printSystemInformation(const SMBIOSSystemInfo* info);
#endif /* B753BF1E_92A3_45A6_ABED_F40B5BE6B059 */
