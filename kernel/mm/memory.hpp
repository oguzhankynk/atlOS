#pragma once

#include <stdint.h>

namespace Memory {
/**
 * @brief
 *
 * @param memHigh
 * @param physicalAllocStart
 */
void init(uintptr_t mbInfoAddr);

void* virtToPhysAddr(void* virtualaddr);

void map(void* physicalAddr, void* virtualAddr);

void unmap(void* virtualaddr);

} // namespace Memory
