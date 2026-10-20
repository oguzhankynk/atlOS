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
    uint64_t ms = shell.context().scheduler.ticks();
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
    const char* unit = run.pageSize == 4096 ? "4K" : (run.pageSize == (1ULL << 30) ? "1G" : "2M");
    printf("  %016lx-%016lx -> %012lx  %6lu MB  %s %s%s %lu x %s\n", run.virtStart, run.virtStart + run.size,
        run.physStart, run.size >> 20, run.flags & 0x2 ? "rw" : "r-", run.flags & 0x4 ? "user" : "kernel",
        run.flags & (1ULL << 63) ? " nx" : "", run.size / run.pageSize, unit);
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
    printf("  virtual                              -> physical        size  access\n");
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
    Threading::Scheduler& scheduler = shell.context().scheduler;
    uint64_t now = scheduler.ticks();

    // The header uses the rows' widths, so the columns line up
    printf("  %c%-3s %-9s %-10s  %-8s  %-35s  %s\n", ' ', "id", "name", "state", "wakes in", "stack", "rsp");
    for (uint32_t i = 0; i < scheduler.taskCount(); i++) {
        const Threading::TCB& tcb = scheduler.task(i);

        char wake[16] = "-";
        if (tcb.state == Threading::ThreadState::Sleeping)
            snprintf(wake, sizeof(wake), "%lu ms", tcb.wakeTick > now ? tcb.wakeTick - now : 0);

        char stack[40] = "boot stack";
        if (tcb.stackBase)
            snprintf(stack, sizeof(stack), "%016lx +%lu KB", tcb.stackBase, tcb.stackSize / 1024);

        // The running task's rsp is live in the CPU; regs.rsp is from its last switch
        printf("  %c%-3u %-9s %-10s  %-8s  %-35s  %016lx\n", i == scheduler.currentIndex() ? '*' : ' ', i,
            tcb.name ? tcb.name : "?", stateName(tcb.state), wake, stack,
            i == scheduler.currentIndex() ? (uint64_t)__builtin_frame_address(0) : tcb.regs.rsp);
    }
    printf("  %u task(s), * = running (this shell)\n", scheduler.taskCount());
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
    { "cpuinfo", "cpuinfo", "CPUID: vendor, features, caches", cmdCpuinfo },
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
