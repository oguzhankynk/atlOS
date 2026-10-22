/**
 * @file process.cpp
 * @author Oguzhan KAYNAK
 * @brief User programs: loading, running in ring 3, system calls, exit.
 */

#include <proc/process.hpp>

#include <stdio.h>
#include <string.h>

#include <core/panic.hpp>
#include <drivers/character_source.hpp>
#include <fs/vfs.hpp>
#include <mm/heap.hpp>
#include <mm/vmm.hpp>
#include <proc/elf.hpp>
#include <sched/scheduler.hpp>
#include <sched/thread.hpp>

extern "C" [[noreturn]] void enterUserMode(uint64_t rip, uint64_t rsp);

namespace Process {

using Memory::AddressSpace;

static Runtime* s_runtime {};

void setRuntime(Runtime* runtime)
{
    s_runtime = runtime;
}

Runtime* runtime()
{
    return s_runtime;
}

/// A program file larger than this is refused (it is read into the heap whole)
static constexpr uint64_t maxFileSize = 16ULL * 1024 * 1024;

/// At most this many bytes per read or write call
static constexpr uint64_t maxTransferSize = 4096;

/// Ctrl+C on the keyboard while a program reads it ends the program
static constexpr char controlC = 0x03;

Runtime::Runtime(Memory::PhysicalMemoryManager& pmm, Memory::AddressSpace& kernelSpace, Fs::Vfs& vfs,
    Drivers::CharacterSource& input)
    : m_pmm(pmm)
    , m_kernelSpace(kernelSpace)
    , m_vfs(vfs)
    , m_input(input)
{
}

// ------------------------------------------------------------
// Loading
// ------------------------------------------------------------

bool Runtime::load(const char* path, Process& process)
{
    Fs::FileInfo info;
    if (!m_vfs.stat(path, info) || info.isDirectory) {
        printf("%s: no such file\n", path);
        return false;
    }
    if (info.size > maxFileSize) {
        printf("%s: larger than %lu MB\n", path, maxFileSize >> 20);
        return false;
    }

    auto* file = static_cast<uint8_t*>(kmalloc(info.size ? info.size : 1));
    if (!file || m_vfs.read(path, 0, file, info.size) != (int64_t)info.size) {
        printf("%s: can not read\n", path);
        kfree(file);
        return false;
    }

    ElfImage image;
    ElfError error = parseElf(file, info.size, image);
    if (error != ElfError::None) {
        printf("%s: %s\n", path, elfErrorName(error));
        kfree(file);
        return false;
    }

    // Each segment: its 2 MB pages (zeroed), then its bytes from the file. The
    // address space is not active: the bytes go in through the physmap
    AddressSpace& space = *process.space;
    for (uint32_t i = 0; i < image.segmentCount; i++) {
        const ElfSegment& segment = image.segments[i];
        uint64_t first = segment.virtAddr & ~(uint64_t)(Memory::pageSize2Mb - 1);
        uint64_t last = (segment.virtAddr + segment.memSize - 1) & ~(uint64_t)(Memory::pageSize2Mb - 1);
        uint64_t flags = AddressSpace::User | (segment.writable ? (uint64_t)AddressSpace::Writable : 0);
        if (!space.allocateAt(first, (last - first) / Memory::pageSize2Mb + 1, flags)) {
            printf("%s: out of memory\n", path);
            kfree(file);
            return false;
        }

        for (uint64_t done = 0; done < segment.fileSize;) {
            uint64_t virt = segment.virtAddr + done;
            uint64_t inPage = Memory::pageSize2Mb - (virt & (Memory::pageSize2Mb - 1));
            uint64_t chunk = segment.fileSize - done < inPage ? segment.fileSize - done : inPage;
            uint64_t phys;
            space.translate((const void*)virt, phys);
            memcpy(Memory::physToVirt(phys), file + segment.fileOffset + done, chunk);
            done += chunk;
        }
    }
    kfree(file);

    // The stack: one writable page at the top of the user range
    uint64_t stackPage = stackTop & ~(uint64_t)(Memory::pageSize2Mb - 1);
    if (!space.allocateAt(stackPage, 1, AddressSpace::User | AddressSpace::Writable)) {
        printf("%s: out of memory for the stack\n", path);
        return false;
    }

    process.entry = image.entry;
    process.stackTop = stackTop;
    return true;
}

int Runtime::spawn(const char* path)
{
    reap();
    if (m_count == maxProcesses) {
        printf("%s: already %u programs running\n", path, maxProcesses);
        return -1;
    }

    auto* process = new Process {};
    process->pid = m_nextPid++;
    // The name is the last path component
    const char* name = path;
    for (const char* c = path; *c; c++) {
        if (*c == '/')
            name = c + 1;
    }
    strncpy(process->name, name, sizeof(process->name) - 1);
    process->space = new AddressSpace(m_pmm, m_kernelSpace);

    if (!load(path, *process)) {
        delete process->space;
        delete process;
        return -1;
    }

    // A kernel thread whose first act is to drop to ring 3. It runs on the
    // program's tables (cr3) and shows the program's name in "tasks"
    Threading::Scheduler& scheduler = Threading::currentScheduler();
    process->thread = new Threading::Thread(scheduler, m_kernelSpace, threadEntry, false);
    Threading::TCB& tcb = process->thread->getTCB();
    tcb.regs.cr3 = process->space->pml4Physical();
    tcb.owner = process;
    tcb.name = process->name;
    scheduler.addTask(*process->thread);

    m_processes[m_count++] = process;
    return (int)process->pid;
}

void Runtime::threadEntry()
{
    auto* process = static_cast<Process*>(Threading::currentTask().owner);
    enterUserMode(process->entry, process->stackTop);
}

void Runtime::reap()
{
    // A program's thread can not free its own stack: the next caller does
    for (uint32_t i = 0; i < m_count;) {
        Process* process = m_processes[i];
        if (!process->exited || process->thread->getTCB().state != Threading::ThreadState::Terminated) {
            i++;
            continue;
        }
        delete process->thread;
        delete process->space;
        delete process;
        m_processes[i] = m_processes[--m_count];
        m_processes[m_count] = nullptr;
    }
}

int64_t Runtime::wait(uint32_t pid)
{
    Process* process = nullptr;
    for (uint32_t i = 0; i < m_count; i++) {
        if (m_processes[i]->pid == pid)
            process = m_processes[i];
    }
    if (!process)
        return -1;

    // exitCurrent gives it, then ends the thread: by the time this one runs
    // again the program's thread has left the queue and can be freed
    process->done.take();
    int64_t code = process->exitCode;
    while (process->thread->getTCB().state != Threading::ThreadState::Terminated)
        Threading::yield();
    reap();
    return code;
}

Process* Runtime::current()
{
    return static_cast<Process*>(Threading::currentTask().owner);
}

const Process* Runtime::find(uint32_t pid) const
{
    for (uint32_t i = 0; i < m_count; i++) {
        if (m_processes[i]->pid == pid)
            return m_processes[i];
    }
    return nullptr;
}

// ------------------------------------------------------------
// System calls and exit
// ------------------------------------------------------------

// The program's path, copied out of its memory byte by byte: every byte must
// be on its user pages, the string must end within maxPath
static int64_t copyPath(Process& process, uint64_t address, char* path)
{
    for (uint32_t i = 0; i < OpenFile::maxPath; i++) {
        if (!process.space->isUserAccessible(address + i, 1, false))
            return ErrorFault;
        path[i] = reinterpret_cast<const char*>(address)[i];
        if (path[i] == '\0')
            return 0;
    }
    return ErrorNameTooLong;
}

OpenFile* Runtime::openFile(Process& process, uint64_t fd)
{
    if (fd < (uint64_t)Process::firstFileDescriptor || fd >= Process::firstFileDescriptor + Process::maxOpenFiles)
        return nullptr;
    OpenFile& file = process.files[fd - Process::firstFileDescriptor];
    return file.used ? &file : nullptr;
}

int64_t Runtime::write(Process& process, uint64_t fd, uint64_t buffer, uint64_t size)
{
    if (size > maxTransferSize)
        return ErrorInvalid;

    // The program may hand any address: only its own user pages are read
    if (!process.space->isUserAccessible(buffer, size, false))
        return ErrorFault;

    // 1 (standard output) and 2 (standard error): the screen
    if (fd == 1 || fd == 2) {
        printf("%.*s", (int)size, reinterpret_cast<const char*>(buffer));
        return (int64_t)size;
    }

    OpenFile* file = openFile(process, fd);
    if (!file || !file->writable)
        return ErrorBadFile;
    if (file->append) {
        Fs::FileInfo info;
        file->offset = m_vfs.stat(file->path, info) ? info.size : 0;
    }
    int64_t written = m_vfs.write(file->path, file->offset, reinterpret_cast<const void*>(buffer), size);
    if (written < 0)
        return ErrorIo;
    file->offset += (uint64_t)written;
    return written;
}

int64_t Runtime::readConsole(char* buffer, uint64_t size)
{
    // Wait for the first key, then take the ones already there. Raw: no
    // echo, no line editing; Enter arrives as '\n' (a terminal sends '\r')
    char c = m_input.read();
    uint64_t count = 0;
    do {
        if (c == controlC) {
            printf("^C\n");
            exitCurrent(128 + 2); // like a shell's 128 + SIGINT
        }
        buffer[count++] = c == '\r' ? '\n' : c;
    } while (count < size && m_input.tryRead(c));
    return (int64_t)count;
}

int64_t Runtime::read(Process& process, uint64_t fd, uint64_t buffer, uint64_t size)
{
    if (size > maxTransferSize)
        return ErrorInvalid;
    if (!process.space->isUserAccessible(buffer, size, true))
        return ErrorFault;
    if (size == 0)
        return 0;

    // 0 (standard input): the keyboard and the serial terminal
    if (fd == 0)
        return readConsole(reinterpret_cast<char*>(buffer), size);

    OpenFile* file = openFile(process, fd);
    if (!file || !file->readable)
        return ErrorBadFile;
    int64_t count = m_vfs.read(file->path, file->offset, reinterpret_cast<void*>(buffer), size);
    if (count < 0)
        return ErrorIo;
    file->offset += (uint64_t)count;
    return count;
}

int64_t Runtime::open(Process& process, uint64_t pathAddress, uint64_t flags)
{
    uint64_t mode = flags & OpenAccessMode;
    if (mode == OpenAccessMode)
        return ErrorInvalid;

    char path[OpenFile::maxPath];
    int64_t copied = copyPath(process, pathAddress, path);
    if (copied < 0)
        return copied;
    // A program has no current directory: paths start at the root
    if (path[0] != '/')
        return ErrorNoEntry;

    OpenFile* file = nullptr;
    for (OpenFile& slot : process.files) {
        if (!slot.used) {
            file = &slot;
            break;
        }
    }
    if (!file)
        return ErrorTooManyFiles;

    bool writable = mode != OpenReadOnly;
    Fs::FileInfo info;
    bool exists = m_vfs.stat(path, info);
    if (exists && info.isDirectory)
        return ErrorIsDirectory;
    if (!exists && !(flags & OpenCreate))
        return ErrorNoEntry;

    // There is no truncate: an emptied file is removed and created again
    if (exists && writable && (flags & OpenTruncate) && !m_vfs.remove(path))
        return ErrorIo;
    if ((!exists || (writable && (flags & OpenTruncate))) && !m_vfs.create(path))
        return ErrorNoEntry; // most likely the directory does not exist

    *file = OpenFile {};
    file->used = true;
    file->readable = mode != OpenWriteOnly;
    file->writable = writable;
    file->append = writable && (flags & OpenAppend);
    strncpy(file->path, path, sizeof(file->path) - 1);
    return Process::firstFileDescriptor + (file - process.files);
}

int64_t Runtime::close(Process& process, uint64_t fd)
{
    OpenFile* file = openFile(process, fd);
    if (!file)
        return ErrorBadFile;
    file->used = false;
    return 0;
}

void Runtime::systemCall(InterruptFrame& frame)
{
    Process* process = current();
    if (!process)
        kernelPanic("int 0x80 from a kernel thread");

    switch (frame.rax) {
    case SysExit:
        exitCurrent((int64_t)frame.rdi);
    case SysWrite:
        frame.rax = (uint64_t)write(*process, frame.rdi, frame.rsi, frame.rdx);
        return;
    case SysRead:
        frame.rax = (uint64_t)read(*process, frame.rdi, frame.rsi, frame.rdx);
        return;
    case SysOpen:
        frame.rax = (uint64_t)open(*process, frame.rdi, frame.rsi);
        return;
    case SysClose:
        frame.rax = (uint64_t)close(*process, frame.rdi);
        return;
    case SysYield:
        Threading::yield();
        frame.rax = 0;
        return;
    case SysSleep:
        Threading::sleep(frame.rdi);
        frame.rax = 0;
        return;
    case SysGetPid:
        frame.rax = process->pid;
        return;
    case SysUptime:
        frame.rax = Threading::currentScheduler().ticks();
        return;
    default:
        frame.rax = (uint64_t)ErrorNoSystemCall;
        return;
    }
}

void Runtime::exitCurrent(int64_t code)
{
    Process* process = current();
    if (!process)
        kernelPanic("exit from a kernel thread");

    process->exited = true;
    process->exitCode = code;
    printf("[pid %u %s exited with %ld]\n", process->pid, process->name, code);
    process->done.give();

    // Back to the kernel's tables: the program's may be freed before this
    // thread is switched away from for the last time
    m_kernelSpace.activate();
    process->thread->getTCB().regs.cr3 = m_kernelSpace.pml4Physical();
    Threading::exitThread();
}

} // namespace Process
