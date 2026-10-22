/**
 * @file commands.cpp
 * @author Oguzhan KAYNAK
 * @brief The shell's commands: one function per command and the table that
 * names them.
 */

#include <shell/interpreter.hpp>

#include <stdio.h>
#include <string.h>

#include <arch/x86_64/cpu/cpu.hpp>
#include <arch/x86_64/cpu/msr.hpp>
#include <arch/x86_64/gdt/gdt.hpp>
#include <arch/x86_64/idt/fault.hpp>
#include <arch/x86_64/idt/idt.hpp>
#include <arch/x86_64/idt/raise.hpp>
#include <arch/x86_64/io/ioport.hpp>
#include <core/panic.hpp>
#include <core/version.hpp>
#include <drivers/disk/block_device.hpp>
#include <drivers/pci/pci.hpp>
#include <drivers/serial.hpp>
#include <drivers/vga_console.hpp>
#include <fs/storage.hpp>
#include <fs/vfs.hpp>
#include <mm/constants.hpp>
#include <mm/heap.hpp>
#include <mm/pmm.hpp>
#include <mm/vmm.hpp>
#include <proc/process.hpp>
#include <sched/scheduler.hpp>
#include <shell/parser.hpp>

namespace Shell {

using Memory::AddressSpace;

// ------------------------------------------------------------
// Helpers
// ------------------------------------------------------------

static bool usage(const char* name, int argc, int min, int max)
{
    if (argc >= min && argc <= max)
        return true;
    for (const Command* command = commands(); command->name; command++) {
        if (!strcmp(command->name, name))
            printf("usage: %s\n", command->usage);
    }
    return false;
}

// path made absolute against the current directory; prints why if it can not be
static bool resolve(Interpreter& shell, const char* path, char* out)
{
    if (shell.resolvePath(path, out, Interpreter::maxPath))
        return true;
    printf("%s: path too long\n", path);
    return false;
}

// ------------------------------------------------------------
// General
// ------------------------------------------------------------

// The groups, in the order help shows them
static const char* const s_groups[] = { "general", "files", "memory", "tasks", "cpu", "devices", "system" };

static void printCommand(const Command& command)
{
    printf("  %-34s %s\n", command.usage, command.help);
}

static void cmdHelp(Interpreter&, int argc, char** argv)
{
    if (!usage("help", argc, 1, 2))
        return;

    // help: one line per group, the command names only
    if (argc == 1) {
        for (const char* group : s_groups) {
            printf("  %-8s", group);
            for (const Command* command = commands(); command->name; command++) {
                if (!strcmp(command->group, group))
                    printf(" %s", command->name);
            }
            printf("\n");
        }
        printf("  \"help <group>\" or \"help <command>\" for the details\n");
        return;
    }

    // help memory: the commands of a group with their usage
    bool found = false;
    for (const Command* command = commands(); command->name; command++) {
        if (!strcmp(command->group, argv[1])) {
            printCommand(*command);
            found = true;
        }
    }
    if (found)
        return;

    // help md: one command
    for (const Command* command = commands(); command->name; command++) {
        if (!strcmp(command->name, argv[1])) {
            printCommand(*command);
            return;
        }
    }
    printf("help: no command or group \"%s\"\n", argv[1]);
}

static void cmdClear(Interpreter& shell, int, char**)
{
    if (shell.context().screen)
        shell.context().screen->clear();
    if (shell.context().serial) {
        static const char s_clear[] = "\x1b[2J\x1b[H"; // ANSI: clear, cursor home
        shell.context().serial->write(s_clear, sizeof(s_clear) - 1);
    }
}

static void cmdVersion(Interpreter&, int, char**)
{
    printf("atlOS %s (%s)\n", ATLOS_VERSION_STRING, ATLOS_GIT_DESCRIBE);
}

static void cmdUptime(Interpreter& shell, int, char**)
{
    uint64_t ms = Threading::currentScheduler().ticks();
    printf("up %lu.%03lu s\n", ms / 1000, ms % 1000);
}

static void cmdEcho(Interpreter& shell, int argc, char** argv)
{
    // "echo words > file" replaces file, ">>" appends to it
    int words = argc;
    const char* target = nullptr;
    bool append = false;
    if (argc >= 3 && (!strcmp(argv[argc - 2], ">") || !strcmp(argv[argc - 2], ">>"))) {
        append = argv[argc - 2][1] == '>';
        target = argv[argc - 1];
        words = argc - 2;
    }

    // The words fit: together they were one line of the editor
    char text[LineEditor::maxLength + 2] {};
    size_t length = 0;
    for (int i = 1; i < words; i++)
        length += snprintf(text + length, sizeof(text) - length, i > 1 ? " %s" : "%s", argv[i]);
    snprintf(text + length, sizeof(text) - length, "\n");

    if (!target) {
        printf("%s", text);
        return;
    }

    char path[Interpreter::maxPath];
    if (!resolve(shell, target, path))
        return;
    Fs::Vfs& vfs = shell.context().vfs;
    Fs::FileInfo info;
    bool exists = vfs.stat(path, info);
    if (exists && info.isDirectory) {
        printf("%s: is a directory\n", path);
        return;
    }
    // No truncate yet: ">" removes the old file and starts a new one
    if (exists && !append) {
        vfs.remove(path);
        exists = false;
    }
    if (!exists && !vfs.create(path)) {
        printf("%s: can not create\n", path);
        return;
    }
    uint64_t offset = append && vfs.stat(path, info) ? info.size : 0;
    if (vfs.write(path, offset, text, strlen(text)) != (int64_t)strlen(text))
        printf("%s: write failed\n", path);
}

// ------------------------------------------------------------
// Files
// ------------------------------------------------------------

static void cmdPwd(Interpreter& shell, int, char**)
{
    printf("%s\n", shell.currentDirectory());
}

static void cmdCd(Interpreter& shell, int argc, char** argv)
{
    if (!usage("cd", argc, 1, 2))
        return;
    const char* path = argc == 2 ? argv[1] : "/";
    if (!shell.changeDirectory(path))
        printf("%s: no such directory\n", path);
}

static void printEntry(const Fs::FileInfo& entry, void* count)
{
    ++*static_cast<int*>(count);
    if (entry.isDirectory)
        printf("  %s/\n", entry.name);
    else
        printf("  %-12s %8lu\n", entry.name, entry.size);
}

static void cmdLs(Interpreter& shell, int argc, char** argv)
{
    if (!usage("ls", argc, 1, 2))
        return;
    char path[Interpreter::maxPath];
    if (!resolve(shell, argc == 2 ? argv[1] : ".", path))
        return;

    int count = 0;
    if (!shell.context().vfs.listDirectory(path, printEntry, &count))
        printf("%s: no such directory\n", path);
    else if (!count)
        printf("  (empty)\n");
}

static void cmdCat(Interpreter& shell, int argc, char** argv)
{
    if (!usage("cat", argc, 2, 2))
        return;
    char path[Interpreter::maxPath];
    if (!resolve(shell, argv[1], path))
        return;

    char buffer[512];
    uint64_t offset = 0;
    while (true) {
        int64_t n = shell.context().vfs.read(path, offset, buffer, sizeof(buffer));
        if (n < 0) {
            printf("%s: can not read\n", path);
            return;
        }
        if (!n)
            return;
        printf("%.*s", (int)n, buffer);
        offset += n;
    }
}

static void cmdTouch(Interpreter& shell, int argc, char** argv)
{
    char path[Interpreter::maxPath];
    if (!usage("touch", argc, 2, 2) || !resolve(shell, argv[1], path))
        return;
    Fs::FileInfo info;
    if (!shell.context().vfs.stat(path, info) && !shell.context().vfs.create(path))
        printf("%s: can not create\n", path);
}

static void cmdMkdir(Interpreter& shell, int argc, char** argv)
{
    char path[Interpreter::maxPath];
    if (!usage("mkdir", argc, 2, 2) || !resolve(shell, argv[1], path))
        return;
    if (!shell.context().vfs.makeDirectory(path))
        printf("%s: can not create\n", path);
}

static void cmdRm(Interpreter& shell, int argc, char** argv)
{
    char path[Interpreter::maxPath];
    if (!usage("rm", argc, 2, 2) || !resolve(shell, argv[1], path))
        return;
    if (!shell.context().vfs.remove(path))
        printf("%s: no such file\n", path);
}

static void cmdRmdir(Interpreter& shell, int argc, char** argv)
{
    char path[Interpreter::maxPath];
    if (!usage("rmdir", argc, 2, 2) || !resolve(shell, argv[1], path))
        return;
    if (!shell.context().vfs.removeDirectory(path))
        printf("%s: not an empty directory\n", path);
}

static void printMount(const Fs::FileInfo& mount, void* vfs)
{
    char path[Interpreter::maxPath];
    snprintf(path, sizeof(path), "/%s", mount.name);
    Fs::FileSystem* fileSystem = static_cast<Fs::Vfs*>(vfs)->find(mount.name);
    printf("  %-10s %s\n", path, fileSystem ? fileSystem->typeName() : "?");
}

static void cmdMount(Interpreter& shell, int, char**)
{
    if (!shell.context().vfs.mountCount())
        printf("  nothing mounted\n");
    shell.context().vfs.listDirectory("/", printMount, &shell.context().vfs);
}

// ------------------------------------------------------------
// Memory
// ------------------------------------------------------------

static void printRange(const char* name, const void* start, const void* end)
{
    uint64_t size = (uint64_t)end - (uint64_t)start;
    printf("  %-8s 0x%016lx - 0x%016lx  %6lu KB\n", name, (uint64_t)start, (uint64_t)end, size / 1024);
}

static void cmdMem(Interpreter& shell, int, char**)
{
    Memory::PhysicalMemoryManager& pmm = shell.context().pmm;
    uint64_t block = Memory::PhysicalMemoryManager::blockSize;
    printf("physical: %lu MB free in %lu blocks of 2 MB (the bitmap covers %lu GB)\n",
        pmm.freeBlockCount() * block / (1024ULL * 1024), pmm.freeBlockCount(),
        pmm.capacity() * block / (1024ULL * 1024 * 1024));

    Memory::HeapStatistics heap = Memory::heapStatistics();
    printf("heap:     %lu KB in use of %lu KB taken from the address space\n", heap.inUse / 1024, heap.reserved / 1024);

    printf("kernel image:\n");
    printRange(".text", &Memory::_text_start, &Memory::_text_end);
    printRange(".rodata", &Memory::_rodata_start, &Memory::_rodata_end);
    printRange(".data", &Memory::_data_start, &Memory::_data_end);
    printRange(".bss", &Memory::_bss_start, &Memory::_bss_end);
}

// Is every byte of [address, address + size) mapped? Says where it stops if not.
static bool isMapped(const AddressSpace& space, uint64_t address, uint64_t size)
{
    uint64_t end = address + size;
    uint64_t physical;
    // One check per 4 KB page (the smallest page there can be)
    for (uint64_t at = address; at < end; at = (at & ~0xFFFULL) + 4096) {
        if (!space.translate((const void*)at, physical)) {
            printf("0x%016lx: not mapped\n", at);
            return false;
        }
        if ((at & ~0xFFFULL) + 4096 == 0)
            break; // the last page of the address space
    }
    return true;
}

// One line of a dump: the address (or offset), 16 bytes in hex, the printable characters
static void printDumpLine(uint64_t label, const uint8_t* data, uint64_t count)
{
    printf("%016lx ", label);
    for (uint64_t i = 0; i < 16; i++) {
        if (i == 8)
            printf(" ");
        if (i < count)
            printf(" %02x", data[i]);
        else
            printf("   ");
    }
    printf("  |");
    for (uint64_t i = 0; i < count; i++)
        printf("%c", data[i] >= 0x20 && data[i] < 0x7F ? data[i] : '.');
    printf("|\n");
}

static void cmdMd(Interpreter& shell, int argc, char** argv)
{
    if (!usage("md", argc, 2, 3))
        return;
    uint64_t address;
    uint64_t length = 128;
    if (!parseNumber(argv[1], address) || (argc == 3 && !parseNumber(argv[2], length)) || !length || length > 4096) {
        printf("md: address and length (1 .. 4096) are numbers, e.g. md 0xb8000 64\n");
        return;
    }
    if (address + length < address || !isMapped(shell.context().space, address, length))
        return;

    // Copied out byte by byte (volatile: device memory must be read as asked)
    const volatile uint8_t* bytes = (const volatile uint8_t*)address;
    for (uint64_t line = 0; line < length; line += 16) {
        uint8_t data[16];
        uint64_t count = length - line < 16 ? length - line : 16;
        for (uint64_t i = 0; i < count; i++)
            data[i] = bytes[line + i];
        printDumpLine(address + line, data, count);
    }
}

static void cmdMw(Interpreter& shell, int argc, char** argv)
{
    if (!usage("mw", argc, 3, 4))
        return;
    uint64_t address;
    uint64_t value;
    uint64_t width = 1;
    if (!parseNumber(argv[1], address) || !parseNumber(argv[2], value) || (argc == 4 && !parseNumber(argv[3], width))
        || (width != 1 && width != 2 && width != 4 && width != 8)) {
        printf("mw: address, value and width (1, 2, 4, 8 bytes) are numbers\n");
        return;
    }
    if (width < 8 && value >> (width * 8)) {
        printf("mw: 0x%lx does not fit into %lu byte(s)\n", value, width);
        return;
    }
    if (address % width) {
        printf("mw: the address must be aligned to the width\n");
        return;
    }
    if (!isMapped(shell.context().space, address, width))
        return;

    switch (width) {
    case 1:
        *(volatile uint8_t*)address = (uint8_t)value;
        break;
    case 2:
        *(volatile uint16_t*)address = (uint16_t)value;
        break;
    case 4:
        *(volatile uint32_t*)address = (uint32_t)value;
        break;
    default:
        *(volatile uint64_t*)address = value;
        break;
    }
}

namespace {
    // Neighbouring pages that continue each other become one line
    struct MappingRun {
        uint64_t virtStart;
        uint64_t physStart;
        uint64_t size;
        uint64_t pageSize;
        uint64_t flags;
        uint32_t lines;
    };
} // namespace

static void printRun(const MappingRun& run)
{
    // One line of at most 80 columns (the VGA screen)
    const char* unit = run.pageSize == 4096 ? "4K" : (run.pageSize == (1ULL << 30) ? "1G" : "2M");
    char size[16];
    if (run.size >= (1ULL << 30))
        snprintf(size, sizeof(size), "%lu GB", run.size >> 30);
    else
        snprintf(size, sizeof(size), "%lu MB", run.size >> 20);
    printf("  %016lx-%016lx %010lx %6s %s %-6s%s %lux%s\n", run.virtStart, run.virtStart + run.size, run.physStart,
        size, run.flags & 0x2 ? "rw" : "r-", run.flags & 0x4 ? "user" : "kernel", run.flags & (1ULL << 63) ? " nx" : "",
        run.size / run.pageSize, unit);
}

static void collectMapping(uint64_t virtAddr, uint64_t physAddr, uint64_t size, uint64_t flags, void* arg)
{
    auto* run = static_cast<MappingRun*>(arg);
    // Only the bits that matter to a reader: writable, user, no execute
    uint64_t kept = flags & (0x2 | 0x4 | (1ULL << 63));

    bool continues = run->size && run->virtStart + run->size == virtAddr && run->physStart + run->size == physAddr
        && run->flags == kept && run->pageSize == size;
    if (continues) {
        run->size += size;
        return;
    }
    if (run->size) {
        printRun(*run);
        run->lines++;
    }
    *run = { virtAddr, physAddr, size, size, kept, run->lines };
}

static void cmdVmmap(Interpreter& shell, int, char**)
{
    printf("  %-33s %-10s %6s %-9s %s\n", "virtual", "physical", "size", "access", "pages");
    MappingRun run {};
    shell.context().space.forEachMapping(collectMapping, &run);
    if (run.size)
        printRun(run);
}

// ------------------------------------------------------------
// Disks
// ------------------------------------------------------------

static void cmdDisk(Interpreter& shell, int argc, char** argv)
{
    Fs::BlockDeviceList& disks = shell.context().disks;

    if (argc == 1) {
        if (!disks.count)
            printf("  no disks\n");
        for (uint32_t i = 0; i < disks.count; i++) {
            Drivers::BlockDevice& device = *disks.devices[i];
            printf("  %-8s %10lu sectors  %6lu MB\n", device.name(), device.sectorCount(), device.sectorCount() / 2048);
        }
        return;
    }

    // disk read lba [count] [device]
    if (strcmp(argv[1], "read") != 0 || argc < 3 || argc > 5) {
        usage("disk", 0, 1, 1);
        return;
    }
    uint64_t lba;
    uint64_t count = 1;
    if (!parseNumber(argv[2], lba) || (argc >= 4 && !parseNumber(argv[3], count)) || !count || count > 8) {
        printf("disk read: a sector number and a count of 1 .. 8\n");
        return;
    }
    Drivers::BlockDevice* device = argc == 5 ? disks.find(argv[4]) : (disks.count ? disks.devices[0] : nullptr);
    if (!device) {
        printf("disk read: %s\n", argc == 5 ? "no such device (\"disk\" lists them)" : "no disks");
        return;
    }

    static uint8_t s_sectors[8 * Drivers::BlockDevice::sectorSize];
    if (!device->readSectors(lba, (uint32_t)count, s_sectors)) {
        printf("disk read: %s can not read sector %lu (it has %lu)\n", device->name(), lba, device->sectorCount());
        return;
    }

    // Labels are byte offsets on the device: sector n starts at n * 512
    for (uint64_t sector = 0; sector < count; sector++) {
        printf("%s sector %lu\n", device->name(), lba + sector);
        for (uint64_t line = 0; line < Drivers::BlockDevice::sectorSize; line += 16) {
            uint64_t offset = sector * Drivers::BlockDevice::sectorSize + line;
            printDumpLine((lba * Drivers::BlockDevice::sectorSize) + offset, s_sectors + offset, 16);
        }
    }
}

// ------------------------------------------------------------
// PCI
// ------------------------------------------------------------

using Drivers::PciAddress;
using Drivers::PciConfigSpace;
using Drivers::PciDevice;

// "bb:dd.f" in hex, e.g. "00:1f.2"
static bool parsePciAddress(const char* text, PciAddress& address)
{
    uint32_t parts[3] {};
    const char separators[3] = { ':', '.', '\0' };
    const uint32_t limits[3] = { 0xFF, 0x1F, 0x7 };
    for (int part = 0; part < 3; part++) {
        int digits = 0;
        for (; *text && *text != separators[part]; text++, digits++) {
            char c = *text;
            uint32_t digit;
            if (c >= '0' && c <= '9')
                digit = c - '0';
            else if (c >= 'a' && c <= 'f')
                digit = c - 'a' + 10;
            else if (c >= 'A' && c <= 'F')
                digit = c - 'A' + 10;
            else
                return false;
            parts[part] = parts[part] * 16 + digit;
        }
        if (!digits || digits > 2 || parts[part] > limits[part] || *text != separators[part])
            return false;
        if (*text)
            text++;
    }
    address = { (uint8_t)parts[0], (uint8_t)parts[1], (uint8_t)parts[2] };
    return true;
}

static void printBars(PciConfigSpace& config, PciAddress address, int count)
{
    for (int i = 0; i < count; i++) {
        uint32_t bar = config.read32(address, (uint8_t)(Drivers::PciBar0 + i * 4));
        if (!bar)
            continue;
        // Bit 0: I/O ports or memory. Memory bits 2..1: 64 bit takes this BAR and the next
        if (bar & 1) {
            printf("  BAR%d      I/O ports at 0x%x\n", i, bar & ~3U);
        } else if (((bar >> 1) & 3) == 2 && i + 1 < count) {
            uint64_t high = config.read32(address, (uint8_t)(Drivers::PciBar0 + (i + 1) * 4));
            printf("  BAR%d      memory at 0x%lx (64 bit%s)\n", i, (high << 32) | (bar & ~0xFU),
                bar & 8 ? ", prefetchable" : "");
            i++;
        } else {
            printf("  BAR%d      memory at 0x%x (32 bit%s)\n", i, bar & ~0xFU, bar & 8 ? ", prefetchable" : "");
        }
    }
}

static void printCapabilities(PciConfigSpace& config, PciAddress address)
{
    if (!(config.read16(address, Drivers::PciStatus) & Drivers::pciStatusCapabilities))
        return;

    // A linked list in the configuration space: each entry is id, next
    printf("  capabilities:");
    uint8_t offset = config.read8(address, Drivers::PciCapabilities) & 0xFC;
    for (int i = 0; offset && i < 48; i++) {
        uint8_t id = config.read8(address, offset);
        const char* name = Drivers::pciCapabilityName(id);
        if (name)
            printf(" %s (0x%02x)", name, offset);
        else
            printf(" 0x%02x (0x%02x)", id, offset);
        offset = config.read8(address, (uint8_t)(offset + 1)) & 0xFC;
    }
    printf("\n");
}

static void printPciDetails(PciConfigSpace& config, const PciDevice& device)
{
    PciAddress address = device.address;
    const char* vendor = Drivers::pciVendorName(device.vendorId);
    printf("%02x:%02x.%x  %s\n", address.bus, address.device, address.function,
        Drivers::pciClassName(device.classCode, device.subclass));
    printf("  vendor    0x%04x %s\n", device.vendorId, vendor ? vendor : "");
    printf("  device    0x%04x  revision 0x%02x\n", device.deviceId, device.revision);
    printf("  class     0x%02x  subclass 0x%02x  prog if 0x%02x\n", device.classCode, device.subclass, device.progIf);

    uint16_t command = config.read16(address, Drivers::PciCommand);
    printf("  command   0x%04x %s%s%s%s\n", command, command & Drivers::PciCommandIo ? " I/O" : "",
        command & Drivers::PciCommandMemory ? " memory" : "",
        command & Drivers::PciCommandBusMaster ? " bus-master" : "",
        command & Drivers::PciCommandNoInterrupts ? " no-INTx" : "");

    // Header type 0: a device with 6 BARs; 1: a bridge to another bus, 2 BARs
    if (device.headerType == 1) {
        printf("  header    1 (PCI to PCI bridge), bus %u behind it\n", config.read8(address, Drivers::PciSecondaryBus));
        printBars(config, address, 2);
    } else {
        printf("  header    %u%s\n", device.headerType, device.headerType == 0 ? " (device)" : "");
        if (device.headerType == 0)
            printBars(config, address, 6);
    }

    uint8_t pin = config.read8(address, Drivers::PciInterruptPin);
    if (pin >= 1 && pin <= 4)
        printf("  interrupt pin INT%c, line %u\n", 'A' + pin - 1, config.read8(address, Drivers::PciInterruptLine));
    printCapabilities(config, address);

    // The standard header, as the registers above read it
    uint8_t header[64];
    for (uint32_t offset = 0; offset < sizeof(header); offset += 4) {
        uint32_t value = config.read32(address, (uint8_t)offset);
        memcpy(header + offset, &value, 4);
    }
    for (uint32_t line = 0; line < sizeof(header); line += 16)
        printDumpLine(line, header + line, 16);
}

static void cmdLspci(Interpreter& shell, int argc, char** argv)
{
    if (!usage("lspci", argc, 1, 2))
        return;

    static PciDevice s_devices[128];
    PciConfigSpace& config = shell.context().pci;
    uint32_t count = Drivers::scanPci(config, s_devices, sizeof(s_devices) / sizeof(s_devices[0]));

    // lspci bb:dd.f: one function in detail
    if (argc == 2) {
        PciAddress address;
        if (!parsePciAddress(argv[1], address)) {
            printf("lspci: an address like 00:1f.2 (bus:device.function, hex)\n");
            return;
        }
        for (uint32_t i = 0; i < count; i++) {
            const PciAddress& found = s_devices[i].address;
            if (found.bus == address.bus && found.device == address.device && found.function == address.function) {
                printPciDetails(config, s_devices[i]);
                return;
            }
        }
        printf("lspci: no function at %s\n", argv[1]);
        return;
    }

    // lspci: one line per function
    for (uint32_t i = 0; i < count; i++) {
        const PciDevice& device = s_devices[i];
        const char* vendor = Drivers::pciVendorName(device.vendorId);
        printf("  %02x:%02x.%x  %04x:%04x  %02x%02x  %-16s %s\n", device.address.bus, device.address.device,
            device.address.function, device.vendorId, device.deviceId, device.classCode, device.subclass,
            vendor ? vendor : "", Drivers::pciClassName(device.classCode, device.subclass));
    }
    printf("  %u function(s)\n", count);
}

// ------------------------------------------------------------
// System
// ------------------------------------------------------------

static const char* stateName(Threading::ThreadState state)
{
    switch (state) {
    case Threading::ThreadState::Running:
        return "running";
    case Threading::ThreadState::Ready:
        return "ready";
    case Threading::ThreadState::Blocked:
        return "blocked";
    case Threading::ThreadState::Sleeping:
        return "sleeping";
    case Threading::ThreadState::Terminated:
        return "terminated";
    }
    return "?";
}

static void cmdTasks(Interpreter& shell, int, char**)
{
    // The shell's own scheduler: the one it runs on
    Threading::Scheduler& scheduler = Threading::currentScheduler();
    uint64_t now = scheduler.ticks();

    // The header uses the rows' widths, so the columns line up
    printf("  %c%-3s %-12s %-9s %-8s %-16s %s\n", ' ', "id", "name", "state", "wakes in", "stack", "rsp");
    for (uint32_t i = 0; i < scheduler.taskCount(); i++) {
        const Threading::TCB& tcb = scheduler.task(i);

        char wake[16] = "-";
        if (tcb.state == Threading::ThreadState::Sleeping)
            snprintf(wake, sizeof(wake), "%lu ms", tcb.wakeTick > now ? tcb.wakeTick - now : 0);

        // Thread stacks are one 2 MB page: its address says enough
        char stack[24] = "boot stack";
        if (tcb.stackBase)
            snprintf(stack, sizeof(stack), "%016lx", tcb.stackBase);

        // The running task's rsp is live in the CPU; regs.rsp is from its last switch
        // A program's thread: rsp is in its kernel stack (it is in a system call or about to start)
        const auto* process = static_cast<const Process::Process*>(tcb.owner);
        char name[24];
        if (process)
            snprintf(name, sizeof(name), "%s:%u", tcb.name, process->pid);
        else
            snprintf(name, sizeof(name), "%s", tcb.name ? tcb.name : "?");

        printf("  %c%-3u %-12s %-9s %-8s %-16s %016lx\n", i == scheduler.currentIndex() ? '*' : ' ', i, name,
            stateName(tcb.state), wake, stack,
            i == scheduler.currentIndex() ? (uint64_t)__builtin_frame_address(0) : tcb.regs.rsp);
    }
    printf("  %u task(s), * = running (this shell)\n", scheduler.taskCount());
}

static void cmdRun(Interpreter& shell, int argc, char** argv)
{
    char path[Interpreter::maxPath];
    if (!usage("run", argc, 2, 2) || !resolve(shell, argv[1], path))
        return;

    // The program runs on its own thread; the shell sleeps until it exits, so
    // the keys typed meanwhile go to the program (Ctrl+C ends it)
    Process::Runtime& runtime = shell.context().runtime;
    int pid = runtime.spawn(path);
    if (pid > 0)
        runtime.wait((uint32_t)pid);
}

// ------------------------------------------------------------
// Hardware: I/O ports, MSRs, control registers, GDT, IDT, page tables, interrupts
// ------------------------------------------------------------

// "b", "w", "l" (1, 2, 4 bytes); b when not given
static bool parseWidth(int argc, char** argv, int index, uint32_t& width)
{
    width = 1;
    if (argc <= index)
        return true;
    if (!strcmp(argv[index], "b"))
        width = 1;
    else if (!strcmp(argv[index], "w"))
        width = 2;
    else if (!strcmp(argv[index], "l"))
        width = 4;
    else
        return false;
    return true;
}

static void cmdIn(Interpreter&, int argc, char** argv)
{
    uint64_t port;
    uint32_t width;
    if (!usage("in", argc, 2, 3))
        return;
    if (!parseNumber(argv[1], port) || port > 0xFFFF || !parseWidth(argc, argv, 2, width)) {
        printf("in: a port 0 .. 0xffff and b, w or l\n");
        return;
    }
    uint32_t value = width == 1 ? IOPort::inportb(port) : (width == 2 ? IOPort::inportw(port) : IOPort::inportl(port));
    printf("port 0x%04lx: 0x%0*x (%u)\n", port, (int)width * 2, value, value);
}

static void cmdOut(Interpreter&, int argc, char** argv)
{
    uint64_t port;
    uint64_t value;
    uint32_t width;
    if (!usage("out", argc, 3, 4))
        return;
    if (!parseNumber(argv[1], port) || port > 0xFFFF || !parseNumber(argv[2], value) || !parseWidth(argc, argv, 3, width)
        || value >> (width * 8)) {
        printf("out: a port 0 .. 0xffff, a value that fits, and b, w or l\n");
        return;
    }
    if (width == 1)
        IOPort::outportb(port, (uint8_t)value);
    else if (width == 2)
        IOPort::outportw(port, (uint16_t)value);
    else
        IOPort::outportl(port, (uint32_t)value);
}

struct BitName {
    uint32_t bit;
    const char* name;
};

// "name" for every set bit that has one
static void printBits(uint64_t value, const BitName* names, uint32_t count)
{
    for (uint32_t i = 0; i < count; i++) {
        if (value & (1ULL << names[i].bit))
            printf(" %s", names[i].name);
    }
}

static constexpr BitName cr0Bits[] = { { 0, "PE" }, { 1, "MP" }, { 2, "EM" }, { 3, "TS" }, { 4, "ET" }, { 5, "NE" },
    { 16, "WP" }, { 18, "AM" }, { 29, "NW" }, { 30, "CD" }, { 31, "PG" } };
static constexpr BitName cr4Bits[] = { { 0, "VME" }, { 1, "PVI" }, { 2, "TSD" }, { 3, "DE" }, { 4, "PSE" }, { 5, "PAE" },
    { 6, "MCE" }, { 7, "PGE" }, { 8, "PCE" }, { 9, "OSFXSR" }, { 10, "OSXMMEXCPT" }, { 11, "UMIP" }, { 13, "VMXE" },
    { 16, "FSGSBASE" }, { 17, "PCIDE" }, { 18, "OSXSAVE" }, { 20, "SMEP" }, { 21, "SMAP" } };
static constexpr BitName eferBits[] = { { 0, "SCE" }, { 8, "LME" }, { 10, "LMA" }, { 11, "NXE" } };
static constexpr BitName entryBits[] = { { 0, "P" }, { 1, "W" }, { 2, "U" }, { 3, "PWT" }, { 4, "PCD" }, { 5, "A" },
    { 6, "D" }, { 7, "PS" }, { 8, "G" }, { 9, "owned" }, { 63, "NX" } };

template <uint32_t N>
static void printRegister(const char* name, uint64_t value, const BitName (&bits)[N])
{
    printf("  %-5s 0x%016lx ", name, value);
    printBits(value, bits, N);
    printf("\n");
}

static void cmdCr(Interpreter&, int, char**)
{
    uint64_t cr0, cr2, cr3, cr4;
    __asm__ volatile("mov %%cr0, %0"
                     : "=r"(cr0));
    __asm__ volatile("mov %%cr2, %0"
                     : "=r"(cr2));
    __asm__ volatile("mov %%cr3, %0"
                     : "=r"(cr3));
    __asm__ volatile("mov %%cr4, %0"
                     : "=r"(cr4));

    printRegister("cr0", cr0, cr0Bits);
    printf("  %-5s 0x%016lx  the address of the last page fault\n", "cr2", cr2);
    printf("  %-5s 0x%016lx  the PML4 (physical)%s%s\n", "cr3", cr3, cr3 & 0x8 ? " PWT" : "", cr3 & 0x10 ? " PCD" : "");
    printRegister("cr4", cr4, cr4Bits);
    uint64_t efer;
    if (readMsr(msrEfer, &efer))
        printRegister("efer", efer, eferBits);
}

static void cmdRdmsr(Interpreter&, int argc, char** argv)
{
    uint64_t msr;
    uint64_t value;
    if (!usage("rdmsr", argc, 2, 2))
        return;
    if (!parseNumber(argv[1], msr) || msr > 0xFFFFFFFF) {
        printf("rdmsr: an MSR number, e.g. rdmsr 0xc0000080 (EFER)\n");
        return;
    }
    // The exception table turns the #GP of an MSR the CPU does not have into false
    if (readMsr((uint32_t)msr, &value))
        printf("msr 0x%08lx: 0x%016lx\n", msr, value);
    else
        printf("msr 0x%08lx: not readable (#GP)\n", msr);
}

static void cmdWrmsr(Interpreter&, int argc, char** argv)
{
    uint64_t msr;
    uint64_t value;
    if (!usage("wrmsr", argc, 3, 3))
        return;
    if (!parseNumber(argv[1], msr) || msr > 0xFFFFFFFF || !parseNumber(argv[2], value)) {
        printf("wrmsr: an MSR number and a value\n");
        return;
    }
    if (!writeMsr((uint32_t)msr, value))
        printf("msr 0x%08lx: not writable (#GP)\n", msr);
}

static void cmdGdt(Interpreter&, int, char**)
{
    struct __attribute__((packed)) {
        uint16_t limit;
        uint64_t base;
    } gdtr;
    __asm__ volatile("sgdt %0"
                     : "=m"(gdtr));
    printf("  GDT at 0x%016lx, %u entries\n", gdtr.base, (gdtr.limit + 1) / 8);

    const auto* entries = reinterpret_cast<const uint64_t*>(gdtr.base);
    for (uint32_t i = 0; i < (uint32_t)(gdtr.limit + 1) / 8; i++) {
        uint64_t e = entries[i];
        uint32_t access = (e >> 40) & 0xFF;
        uint32_t flags = (e >> 52) & 0xF;
        uint64_t base = ((e >> 16) & 0xFFFFFF) | ((e >> 32) & 0xFF000000);
        uint32_t limit = (e & 0xFFFF) | ((e >> 32) & 0xF0000);
        bool present = access & 0x80;
        uint32_t dpl = (access >> 5) & 3;

        printf("  0x%02x ", i * 8);
        if (!present) {
            printf("(not present)\n");
            continue;
        }
        if (access & 0x10) { // code or data
            bool code = access & 0x08;
            printf("%s  dpl %u%s%s\n", code ? "code" : "data", dpl, code && (flags & 0x2) ? "  64-bit" : "",
                code ? ((access & 0x02) ? "  readable" : "") : ((access & 0x02) ? "  writable" : ""));
            continue;
        }
        // A system descriptor takes two slots in long mode: base bits 32..63 in the second
        uint32_t type = access & 0xF;
        if (type == 0x9 || type == 0xB) {
            base |= (entries[i + 1] & 0xFFFFFFFF) << 32;
            printf("TSS (%s)  base 0x%016lx  limit 0x%x  dpl %u\n", type == 0xB ? "busy" : "available", base, limit, dpl);
            i++;
            continue;
        }
        printf("system type 0x%x\n", type);
    }
}

// What a vector is: an exception, a PIC IRQ (0x20 + line), or the system call gate
static void vectorName(uint32_t vector, char* name, size_t size)
{
    static const char* const s_irqs[16] = { "timer", "keyboard", "cascade", "COM2", "COM1", "LPT2", "floppy", "LPT1",
        "RTC", "IRQ 9", "IRQ 10", "IRQ 11", "PS/2 mouse", "FPU", "primary ATA", "secondary ATA" };
    if (Fault::exceptionName(vector))
        snprintf(name, size, "%s", Fault::exceptionName(vector));
    else if (vector >= 0x20 && vector < 0x30)
        snprintf(name, size, "IRQ %u %s", vector - 0x20, s_irqs[vector - 0x20]);
    else if (vector == IDT::systemCallVector)
        snprintf(name, size, "system call");
    else
        name[0] = '\0';
}

static void printIdtEntry(uint32_t vector)
{
    const IDT::IDTEntry& e = IDT::entry(vector);
    uint64_t handler = e.isrLow | ((uint64_t)e.isrMid << 16) | ((uint64_t)e.isrHigh << 32);
    uint32_t type = e.attributes & 0xF;
    char name[32];
    vectorName(vector, name, sizeof(name));
    if (!(e.attributes & 0x80)) {
        printf("  %3u  (no gate)  %s\n", vector, name);
        return;
    }
    printf("  %3u  0x%016lx  %-4s dpl %u  ist %u  %s\n", vector, handler, type == 0xF ? "trap" : "int",
        (e.attributes >> 5) & 3, e.ist, name);
}

static void cmdIdt(Interpreter&, int argc, char** argv)
{
    if (!usage("idt", argc, 1, 2))
        return;
    if (argc == 2) {
        uint64_t vector;
        if (!parseNumber(argv[1], vector) || vector > 255) {
            printf("idt: a vector 0 .. 255\n");
            return;
        }
        printIdtEntry(vector);
        return;
    }

    // The exceptions, the PIC IRQs and the system call gate; the rest go to the default stubs
    printf("  vec  handler             type dpl    ist\n");
    for (uint32_t v = 0; v < 0x30; v++)
        printIdtEntry(v);
    printIdtEntry(IDT::systemCallVector);
    printf("  (48 .. 255: interrupt gates, no handler attached; \"idt n\" shows one)\n");
}

static void cmdPt(Interpreter& shell, int argc, char** argv)
{
    uint64_t address;
    if (!usage("pt", argc, 2, 2))
        return;
    if (!parseNumber(argv[1], address)) {
        printf("pt: a virtual address\n");
        return;
    }

    static const char* const s_levels[4] = { "PML4", "PDPT", "PD", "PT" };
    static const uint64_t s_pageSizes[4] = { 0, 1ULL << 30, 2ULL << 20, 4096 };
    Memory::AddressSpace::WalkStep steps[4] {};
    uint32_t count = shell.context().space.walk(address, steps);
    if (!count)
        return; // walk reads at least the PML4

    for (uint32_t level = 0; level < count; level++) {
        const Memory::AddressSpace::WalkStep& step = steps[level];
        char where[16];
        snprintf(where, sizeof(where), "%s[%u]", s_levels[level], step.index);
        printf("  %-10s = 0x%016lx ", where, step.entry);
        printBits(step.entry, entryBits, sizeof(entryBits) / sizeof(entryBits[0]));
        printf("\n");
    }

    const Memory::AddressSpace::WalkStep& last = steps[count - 1];
    if (!(last.entry & 0x1)) {
        printf("  not mapped: %s[%u] is not present\n", s_levels[count - 1], last.index);
        return;
    }
    uint64_t pageSize = s_pageSizes[count - 1];
    uint64_t physical = (last.entry & 0x000FFFFFFFFFF000ULL & ~(pageSize - 1)) | (address & (pageSize - 1));
    printf("  %s page, physical 0x%lx\n", pageSize == 4096 ? "4 KB" : (pageSize == (2ULL << 20) ? "2 MB" : "1 GB"),
        physical);
}

static void cmdInt(Interpreter&, int argc, char** argv)
{
    uint64_t vector;
    if (!usage("int", argc, 2, 2))
        return;
    if (!parseNumber(argv[1], vector) || vector > 255) {
        printf("int: a vector 0 .. 255\n");
        return;
    }

    char name[32];
    vectorName(vector, name, sizeof(name));
    if (vector == 3)
        printf("int 3: a breakpoint: it reports and goes on\n");
    else if (vector < 32)
        printf("int %lu: %s, an exception in the kernel: it reports and halts\n", vector, name);
    else if (vector == IDT::systemCallVector)
        printf("int 0x80 from the kernel is not a system call (only ring 3 makes one)\n");

    // The handler runs as if the CPU or the device had raised it; irq counts it
    IDT::raise((uint8_t)vector);
}

static void cmdIrq(Interpreter&, int, char**)
{
    printf("  vec  count       what\n");
    for (uint32_t v = 0; v < IDT::idtTableLen; v++) {
        uint64_t count = IDT::interruptCount(v);
        if (!count)
            continue;
        char name[32];
        vectorName(v, name, sizeof(name));
        printf("  %3u  %-10lu  %s\n", v, count, name);
    }
}

static void cmdCpuinfo(Interpreter&, int, char**)
{
    static CPUInfo s_info;
    initializeCpuInfo(&s_info);
    printCPUInfo(&s_info);
}

static void cmdPanic(Interpreter&, int argc, char** argv)
{
    kernelPanic(argc > 1 ? argv[1] : "panic command");
}

__attribute__((noinline)) static void readUnmapped()
{
    // Above the identity mapped boot memory, far below the kernel: not mapped
    printf("reading 0xdead0000...\n");
    uint64_t value = *reinterpret_cast<volatile uint64_t*>(0xDEAD0000);
    printf("read 0x%lx, expected a page fault\n", value);
}

static void cmdFault(Interpreter&, int, char**)
{
    readUnmapped();
}

static void cmdReboot(Interpreter&, int, char**)
{
    printf("rebooting\n");

    // The 8042 keyboard controller can pull the CPU's reset line (command 0xFE)
    for (int i = 0; i < 100000 && (IOPort::inportb(0x64) & 0x02); i++)
        ;
    IOPort::outportb(0x64, 0xFE);

    // Did not work: an empty IDT makes the next interrupt a triple fault, a reset as well
    struct __attribute__((packed)) {
        uint16_t limit;
        uint64_t base;
    } emptyIdt = { 0, 0 };
    __asm__ volatile("lidt %0; int3"
                     :
                     : "m"(emptyIdt));
}

static const Command s_commands[] = {
    { "general", "help", "help [group | command]", "the commands, by group", cmdHelp },
    { "general", "clear", "clear", "clear the screen", cmdClear },
    { "general", "echo", "echo [words] [> | >> file]", "print words, or write them to a file", cmdEcho },
    { "general", "version", "version", "the kernel version", cmdVersion },
    { "general", "uptime", "uptime", "time since boot", cmdUptime },
    { "files", "pwd", "pwd", "the current directory", cmdPwd },
    { "files", "cd", "cd [dir]", "change the current directory (default /)", cmdCd },
    { "files", "ls", "ls [dir]", "list a directory", cmdLs },
    { "files", "cat", "cat file", "print a file", cmdCat },
    { "files", "touch", "touch file", "create an empty file", cmdTouch },
    { "files", "mkdir", "mkdir dir", "create a directory", cmdMkdir },
    { "files", "rm", "rm file", "delete a file", cmdRm },
    { "files", "rmdir", "rmdir dir", "delete an empty directory", cmdRmdir },
    { "files", "mount", "mount", "the mounted file systems", cmdMount },
    { "memory", "mem", "mem", "physical memory, heap, kernel image", cmdMem },
    { "memory", "md", "md addr [len]", "memory dump (hex and text), len up to 4096", cmdMd },
    { "memory", "mw", "mw addr value [1|2|4|8]", "write a value of 1 (default) to 8 bytes", cmdMw },
    { "memory", "vmmap", "vmmap", "the page table mappings", cmdVmmap },
    { "tasks", "tasks", "tasks", "the scheduler's tasks: name, state, stack", cmdTasks },
    { "tasks", "ps", "ps", "the same as tasks", cmdTasks },
    { "tasks", "run", "run program.elf", "run a user program (ring 3) until it exits", cmdRun },
    { "cpu", "cpuinfo", "cpuinfo", "CPUID: vendor, features, caches", cmdCpuinfo },
    { "cpu", "cr", "cr", "control registers and EFER, bits by name", cmdCr },
    { "cpu", "rdmsr", "rdmsr msr", "read a model specific register", cmdRdmsr },
    { "cpu", "wrmsr", "wrmsr msr value", "write a model specific register", cmdWrmsr },
    { "devices", "in", "in port [b|w|l]", "read an I/O port (byte, word, long)", cmdIn },
    { "devices", "out", "out port value [b|w|l]", "write an I/O port", cmdOut },
    { "cpu", "gdt", "gdt", "the GDT: segments and the TSS", cmdGdt },
    { "cpu", "idt", "idt [vector]", "the IDT gates: handler, type, DPL, IST", cmdIdt },
    { "memory", "pt", "pt addr", "the page table walk for an address", cmdPt },
    { "devices", "irq", "irq", "how often each vector has arrived", cmdIrq },
    { "devices", "disk", "disk [read lba [count] [device]]", "the disks; dump sectors (count 1 .. 8)", cmdDisk },
    { "devices", "lspci", "lspci [bus:dev.fn]", "the PCI functions; one in detail", cmdLspci },
    { "devices", "int", "int vector", "raise an interrupt (3 reports, other exceptions halt)", cmdInt },
    { "system", "panic", "panic [message]", "kernel panic (halts)", cmdPanic },
    { "system", "fault", "fault", "read an unmapped address: page fault (halts)", cmdFault },
    { "system", "reboot", "reboot", "reset the machine", cmdReboot },
    { nullptr, nullptr, nullptr, nullptr, nullptr },
};

const Command* commands()
{
    return s_commands;
}

} // namespace Shell
