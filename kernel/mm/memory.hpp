#pragma once

#include <stdint.h>

namespace Memory {

class PhysicalMemoryManager;

/**
 * @brief Print the kernel's boot, section and physical/virtual layout.
 */
void printLayout();

/**
 * @brief Add the AVAILABLE regions of the multiboot2 memory map to pmm, then
 * reserve low memory and the kernel image (below kernel_physical_end) and the
 * multiboot info itself, which the firmware reports as available too.
 *
 * @param mbInfoAddr Address of the multiboot2 information structure; it is read
 *                   through this address and reserved as a physical range.
 */
void populateFromMultiboot(PhysicalMemoryManager& pmm, uintptr_t mbInfoAddr);

} // namespace Memory
