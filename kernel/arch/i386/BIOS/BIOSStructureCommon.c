#include <string.h>
#include <kernel/private/BIOS/BIOSStructureCommon.h>

void extractStringAddress(const char *str_start, const char *str_end, const char *addresses[], unsigned int MAX_STRINGS)
{
    unsigned int count = 0;
    while (*str_start != '\0' && count < MAX_STRINGS && str_start<=str_end)
    {
        addresses[count++] = str_start;
        str_start += strlen(str_start) + 1; // Bir sonraki dizeye geç
    }
}