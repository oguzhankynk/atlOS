/**
 * @file process.cpp
 * @author Oguzhan KAYNAK
 * @brief User programs: loading, running in ring 3, system calls, exit.
 */

#include <proc/process.hpp>

#include <stdio.h>
#include <string.h>

#include <core/panic.hpp>
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

/// At most this many bytes per write call
static constexpr uint64_t maxWriteSize = 4096;

Runtime::Runtime(Memory::PhysicalMemoryManager& pmm, Memory::AddressSpace& kernelSpace, Fs::Vfs& vfs)
    : m_pmm(pmm)
    , m_kernelSpace(kernelSpace)
    , m_vfs(vfs)
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

int64_t Runtime::write(Process& process, uint64_t fd, uint64_t buffer, uint64_t size)
{
    if (fd != 1)
        return ErrorBadFile;
    if (size > maxWriteSize)
        return ErrorInvalid;

    // The program may hand any address: only its own user pages are read
    if (!process.space->isUserAccessible(buffer, size, false))
        return ErrorFault;

    printf("%.*s", (int)size, reinterpret_cast<const char*>(buffer));
    return (int64_t)size;
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

    // Back to the kernel's tables: the program's may be freed before this
    // thread is switched away from for the last time
    m_kernelSpace.activate();
    process->thread->getTCB().regs.cr3 = m_kernelSpace.pml4Physical();
    Threading::exitThread();
}

} // namespace Process
