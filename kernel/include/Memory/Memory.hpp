#ifndef AA43D675_0DC2_4B6E_8A42_68A48886962B
#define AA43D675_0DC2_4B6E_8A42_68A48886962B

#include <stdint.h>

struct multiboot_info;
typedef struct multiboot_info multiboot_info_t;

namespace Memory
{
    /**
     * @brief 
     * 
     * @param memHigh 
     * @param physicalAllocStart 
     */
    void Init(const multiboot_info_t *bootInfo);

    void *VirtToPhysAddr(void *virtualaddr);

} // namespace Memory

#endif /* AA43D675_0DC2_4B6E_8A42_68A48886962B */
