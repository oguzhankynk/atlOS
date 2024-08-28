/**
 * @file kernel.cpp
 * @author Oguzhan KAYNAK
 * @brief Kernel implementation file containing kernel initialization and utility functions.
 *
 * This file contains the implementation of kernel initialization and various utility functions
 * used by the kernel during runtime. It includes functions for hardware initialization,
 * interrupt handling, memory management, and other essential operations.
 * Additionally, it may contain device drivers, system calls, and other kernel-specific functionalities.
 *
 * Copyright © 2024 Oguzhan KAYNAK
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the “Software”), to deal in the Software
 * without  restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit
 * persons to whom the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
 * PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */
#include <stdio.h>
#include <string.h>

#include <arch/i386/GDT/gdt.hpp>
#include <arch/i386/IDT/idt.hpp>

#include <kernel/tty.h>
#include <kernel/BIOS/BIOS.h>
#include <kernel/sys/multiboot.h>
#include <kernel/sys/IOPort.hpp>
#include <kernel/panic.h>
#include <kernel/banner.h>

#include <Memory/Memory.hpp>
#include <Memory/PhysicalMemory.hpp>
#include <Memory/KHeap.h>

#include <hardware/pic.hpp>
#include <Interfaces/serial.hpp>
#include <Interfaces/ATA.hpp>
#include <Drivers/keyboard.hpp>

#include <FileSystem/VFS.hpp>
#include <FileSystem/FAT12.hpp>


void read_boot_sector() {
    uint8_t boot_sector[512];

    // LBA 0 (boot sektörü) oku
    ATA::ata_read_sector(0, boot_sector);

    // Boot sektörü verisini incele veya işle
    printf("Boot Sector: \n");
    for (int i = 0; i < 512; i++) {
        printf("%02X ", boot_sector[i]);
        if ((i + 1) % 16 == 0) {
            printf("\n");
        }
    }

    // İşletim sistemi imzasını kontrol et (0x55AA)
    if (boot_sector[510] == 0x55 && boot_sector[511] == 0xAA) {
        printf("Boot sector signature found.\n");
    }
    else {
        printf("Boot sector signature not found.\n");
    }
}

/**
 * @brief entry point of atlOS
 *
 */
extern "C" void kernel_main(uint32_t multiboot_bootloader_magic, uint32_t multiboot_info_addr)
{

    terminal_initialize();

    atlOSBanner();


    GDT::Init();

    IDT::Init();

    if (multiboot_bootloader_magic != MULTIBOOT_BOOTLOADER_MAGIC)
    {
        kernel_panic("magic number is invalid.");
    }


    Memory::Init((multiboot_info_t*)multiboot_info_addr);

    //asm volatile("int $0x00");

    PIC::Init();

    Keyboard::Init();

    int stat_serial_init = SerialPort::Init();

    printf("stat_serial_init: %d \n", stat_serial_init);

    struct PartitionEntry {
        uint8_t  boot_flag;           // Boot flag (0x80 = aktif, 0x00 = inaktif)
        uint8_t  chs_start[3];        // Başlangıç CHS adresi
        uint8_t  partition_type;      // Bölüm tipi (örneğin, 0x0B = FAT32)
        uint8_t  chs_end[3];          // Bitiş CHS adresi
        uint32_t lba_start;           // Başlangıç LBA adresi
        uint32_t sectors_count;       // Sektör sayısı
    } __attribute__((packed));

    struct MBR {
        uint8_t          bootloader_code[446];  // Bootloader kodu veya diğer veriler
        PartitionEntry   partition_table[4];    // 4 adet bölüm girişi
        uint16_t         magic_number;          // Sihirli sayı (0x55AA)
    } __attribute__((packed));

    MBR mbr;
    //! read boot sector
    ATA::ata_read_sector(0, (uint8_t*)&mbr);


     printf("MBR Information:\n");

    // Sihirli sayıyı kontrol et
    if (mbr.magic_number == 0xAA55) {
        printf(" - Valid MBR (Magic Number 0x55AA found)\n");
    } else {
        printf(" - Invalid MBR (Magic Number not found)\n");
    }

    // Bölüm tablosunu kontrol et
    for (int i = 0; i < 4; i++) {
        const PartitionEntry& entry = mbr.partition_table[i];
        if (entry.partition_type != 0x00) { // 0x00 = boş bölüm girişi
            printf(" - Partition %d:\n", i + 1);
            printf("   - Boot Flag: %s\n", (entry.boot_flag == 0x80 ? "Yes" : "No"));
            printf("   - Type: 0x%02X\n", entry.partition_type);
            printf("   - LBA Start: %u\n", entry.lba_start);
            printf("   - Sector Count: %u\n", entry.sectors_count);
        }
    }

    while (1)
        ;
}
