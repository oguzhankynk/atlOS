#ifndef AA43D675_0DC2_4B6E_8A42_68A48886962B
#define AA43D675_0DC2_4B6E_8A42_68A48886962B

#include <stdint.h>


namespace Memory
{
    /**
     * @brief
     *
     * @param memHigh
     * @param physicalAllocStart
     */
    void init(uintptr_t mbInfoAddr);

    void *virtToPhysAddr(void *virtualaddr);

    void map(void *physicalAddr, void *virtualAddr);

    void unmap(void *virtualaddr);

} // namespace Memory

#endif /* AA43D675_0DC2_4B6E_8A42_68A48886962B */
