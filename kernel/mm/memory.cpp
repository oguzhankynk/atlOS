#include <mm/memory.hpp>

#include <stdio.h>
#include <string.h>

#include <mm/constants.hpp>
#include <mm/pmm.hpp>
#include <mm/vmm.hpp>

#include <arch/x86_64/boot/multiboot2.inc>
#include <arch/x86_64/idt/handlers.hpp>
#include <arch/x86_64/mmu/mmu.inc>

namespace Memory {

static void printSegmentAddresses()
{
    printf("=== Boot Segments ===\n");
    printf("_boottext: %p - %p, size: 0x%X\n", &_boottext_start, &_boottext_end, &_boottext_end - &_boottext_start);
    printf("_bootrodata: %p - %p, size: 0x%X\n", &_bootrodata_start, &_bootrodata_end, &_bootrodata_end - &_bootrodata_start);
    printf("_bootdata: %p - %p, size: 0x%X\n", &_bootdata_start, &_bootdata_end, &_bootdata_end - &_bootdata_start);
    printf("_bootbss: %p - %p, size: 0x%X\n", &_bootbss_start, &_bootbss_end, &_bootbss_end - &_bootbss_start);

    printf("\n=== Kernel Segments (VMA) ===\n");
    printf("_text: %p - %p, size: 0x%X\n", &_text_start, &_text_end, &_text_end - &_text_start);
    printf("_rodata: %p - %p, size: 0x%X\n", &_rodata_start, &_rodata_end, &_rodata_end - &_rodata_start);
    printf("_data: %p - %p, size: 0x%X\n", &_data_start, &_data_end, &_data_end - &_data_start);
    printf("_bss: %p - %p, size: 0x%X\n", &_bss_start, &_bss_end, &_bss_end - &_bss_start);

    printf("__ctors: %p - %p, size: 0x%zx\n",
        (void*)__ctors_start,
        (void*)__ctors_end,
        (uintptr_t)__ctors_end - (uintptr_t)__ctors_start);
    printf("__dtors: %p - %p, size: 0x%zx\n",
        (void*)__dtors_start,
        (void*)__dtors_end,
        (uintptr_t)__dtors_end - (uintptr_t)__dtors_start);

    printf("\n=== Kernel Start/End ===\n");
    printf("kernel_virtual_start: %p, kernel_virtual_end: %p, size: 0x%X\n", (uint64_t)&kernel_virtual_start, (uint64_t)&kernel_virtual_end, (uint64_t)&kernel_virtual_start - (uint64_t)&kernel_virtual_end);

    auto virtIndxStart = PML4_ADDR_TO_ENTRY_INDEX((uint64_t)&kernel_virtual_start);
    auto virtIndxEnd = PML4_ADDR_TO_ENTRY_INDEX((uint64_t)&kernel_virtual_end);
    printf("Kernel virtual start PML4 index: %llu\n", virtIndxStart);
    printf("Kernel virtual end PML4 index: %llu\n", virtIndxEnd);

    printf("\n=== Kernel Physical ===\n");
    printf("kernel_physical_start: %p, kernel_physical_end: %p, size: 0x%X\n", (uint64_t)&kernel_physical_start, (uint64_t)&kernel_physical_end, (uint64_t)&kernel_physical_end - (uint64_t)&kernel_physical_start);
}

/**
 * @brief
 *
 * @param memHigh
 * @param physicalAllocStart
 */
void init(uintptr_t mbInfoAddr)
{
    printSegmentAddresses();

    PhysicalMemory::init();
    VirtualMemory::init();

    multiboot_tag* tag;
    uint8_t* addr = (uint8_t*)(uintptr_t)mbInfoAddr;

    uint32_t totalSize = *(uint32_t*)addr;
    addr += 8; // totalSize + reserved

    for (tag = (multiboot_tag*)addr;
         tag->type != MULTIBOOT_TAG_TYPE_END;
         tag = (multiboot_tag*)((uint8_t*)tag + ((tag->size + 7) & ~7))) {
        if (tag->type == MULTIBOOT_TAG_TYPE_MMAP) {
            multiboot_tag_mmap* mmap = (multiboot_tag_mmap*)tag;

            for (multiboot_mmap_entry* entry = mmap->entries;
                 (uint8_t*)entry < (uint8_t*)mmap + mmap->size;
                 entry = (multiboot_mmap_entry*)((uint8_t*)entry + mmap->entry_size)) {
                const char* typeStr;
                switch (entry->type) {
                case MULTIBOOT_MEMORY_AVAILABLE:
                    typeStr = "AVAILABLE";
                    break;
                case MULTIBOOT_MEMORY_RESERVED:
                    typeStr = "RESERVED";
                    break;
                case MULTIBOOT_MEMORY_ACPI_RECLAIMABLE:
                    typeStr = "ACPI_RECLAIMABLE";
                    break;
                case MULTIBOOT_MEMORY_NVS:
                    typeStr = "NVS";
                    break;
                case MULTIBOOT_MEMORY_BADRAM:
                    typeStr = "BADRAM";
                    break;
                default:
                    typeStr = "UNKNOWN";
                    break;
                }

                printf("Addr: 0x%016llx | Length: 0x%016llx | Type: %s\n",
                    (unsigned long long)entry->addr,
                    (unsigned long long)entry->len,
                    typeStr);

                if (entry->type == MULTIBOOT_MEMORY_AVAILABLE) {
                    // 2MB align
                    uint64_t base = (entry->addr + (2 * 1024 * 1024 - 1)) & ~(2 * 1024 * 1024 - 1);
                    uint64_t end = (entry->addr + entry->len) & ~(2 * 1024 * 1024 - 1);
                    if (end > base) {
                        PhysicalMemory::addMemoryRegion((void*)base, end - base);
                    }
                }
            }
        }
    }

    // The firmware reports the memory holding the kernel as AVAILABLE. Reserve
    // everything below kernel_physical_end (low memory, boot page tables and
    // stack, kernel image) so the PMM never hands those frames out.
    PhysicalMemory::removeMemoryRegion(nullptr, (uint64_t)&kernel_physical_end);

    // GRUB may place the multiboot info outside the kernel image
    PhysicalMemory::removeMemoryRegion((void*)mbInfoAddr, totalSize);
}

/**
 * @brief Converts a virtual address to a physical address.
 *
 * @param virtualaddr The virtual address to be converted.
 * @return void* The corresponding physical address, or nullptr if the address is not present.
 */
void* virtToPhysAddr(void* virtualaddr)
{
    return VirtualMemory::virtToPhys(virtualaddr);
}

/**
 * @brief Map a 2 MB page at virtualaddr to physicaladdr. Both must be 2 MB aligned.
 */
void map(void* physicaladdr, void* virtualaddr)
{
    VirtualMemory::mapPhysical(virtualaddr, physicaladdr);
}

/**
 * @brief Remove the 2 MB page containing virtualaddr. The physical memory is not freed.
 */
void unmap(void* virtualaddr)
{
    VirtualMemory::unmapPhysical(virtualaddr);
}

} // namespace Memory
