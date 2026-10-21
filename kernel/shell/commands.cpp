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
#include <drivers/serial.hpp>
#include <drivers/vga_console.hpp>
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

static void cmdHelp(Interpreter&, int, char**)
{
    for (const Command* command = commands(); command->name; command++)
        printf("  %-28s %s\n", command->usage, command->help);
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

    // 16 bytes a line: address, hex, the printable characters
    const volatile uint8_t* bytes = (const volatile uint8_t*)address;
    for (uint64_t line = 0; line < length; line += 16) {
        uint8_t data[16];
        uint64_t count = length - line < 16 ? length - line : 16;
        for (uint64_t i = 0; i < count; i++)
            data[i] = bytes[line + i];

        printf("%016lx ", address + line);
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

    // The program starts on its own thread and runs while the shell waits for keys
    int pid = shell.context().runtime.spawn(path);
    if (pid > 0)
        printf("started pid %d\n", pid);
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
    { "help", "help", "this list", cmdHelp },
    { "clear", "clear", "clear the screen", cmdClear },
    { "echo", "echo [words] [> | >> file]", "print words, or write them to a file", cmdEcho },
    { "version", "version", "the kernel version", cmdVersion },
    { "uptime", "uptime", "time since boot", cmdUptime },
    { "pwd", "pwd", "the current directory", cmdPwd },
    { "cd", "cd [dir]", "change the current directory (default /)", cmdCd },
    { "ls", "ls [dir]", "list a directory", cmdLs },
    { "cat", "cat file", "print a file", cmdCat },
    { "touch", "touch file", "create an empty file", cmdTouch },
    { "mkdir", "mkdir dir", "create a directory", cmdMkdir },
    { "rm", "rm file", "delete a file", cmdRm },
    { "rmdir", "rmdir dir", "delete an empty directory", cmdRmdir },
    { "mount", "mount", "the mounted file systems", cmdMount },
    { "mem", "mem", "physical memory, heap, kernel image", cmdMem },
    { "md", "md addr [len]", "memory dump (hex and text), len up to 4096", cmdMd },
    { "mw", "mw addr value [1|2|4|8]", "write a value of 1 (default) to 8 bytes", cmdMw },
    { "vmmap", "vmmap", "the page table mappings", cmdVmmap },
    { "tasks", "tasks", "the scheduler's tasks: name, state, stack", cmdTasks },
    { "ps", "ps", "the same as tasks", cmdTasks },
    { "run", "run program.elf", "start a user program (ring 3) on its own thread", cmdRun },
    { "cpuinfo", "cpuinfo", "CPUID: vendor, features, caches", cmdCpuinfo },
    { "cr", "cr", "control registers and EFER, bits by name", cmdCr },
    { "rdmsr", "rdmsr msr", "read a model specific register", cmdRdmsr },
    { "wrmsr", "wrmsr msr value", "write a model specific register", cmdWrmsr },
    { "in", "in port [b|w|l]", "read an I/O port (byte, word, long)", cmdIn },
    { "out", "out port value [b|w|l]", "write an I/O port", cmdOut },
    { "gdt", "gdt", "the GDT: segments and the TSS", cmdGdt },
    { "idt", "idt [vector]", "the IDT gates: handler, type, DPL, IST", cmdIdt },
    { "pt", "pt addr", "the page table walk for an address", cmdPt },
    { "irq", "irq", "how often each vector has arrived", cmdIrq },
    { "int", "int vector", "raise an interrupt (3 reports, other exceptions halt)", cmdInt },
    { "panic", "panic [message]", "kernel panic (halts)", cmdPanic },
    { "fault", "fault", "read an unmapped address: page fault (halts)", cmdFault },
    { "reboot", "reboot", "reset the machine", cmdReboot },
    { nullptr, nullptr, nullptr, nullptr },
};

const Command* commands()
{
    return s_commands;
}

} // namespace Shell
