/**
 * @file process.hpp
 * @author Oguzhan KAYNAK
 * @brief User programs: loading a static ELF64 file into an address space of
 * its own and running it in ring 3 on a kernel thread; the int 0x80 system
 * calls; ending a program on exit or on a fault.
 */

#pragma once

#include <stdint.h>

#include <arch/x86_64/cpu/cpu.hpp>

namespace Fs {
class Vfs;
}

namespace Memory {
class AddressSpace;
class PhysicalMemoryManager;
}

namespace Threading {
class Scheduler;
class Thread;
}

namespace Process {

/// The system call numbers (rax); arguments in rdi, rsi, rdx; the result in rax
enum SystemCall : uint64_t {
    SysExit = 0, ///< exit(code): end the program
    SysWrite = 1, ///< write(fd, buffer, size): fd 1 (standard output) only
};

/// Negative results of a system call (the Linux numbers)
enum SystemCallError : int64_t {
    ErrorBadFile = -9, ///< EBADF
    ErrorFault = -14, ///< EFAULT: a pointer the program may not use
    ErrorInvalid = -22, ///< EINVAL
    ErrorNoSystemCall = -38, ///< ENOSYS
};

struct Process {
    uint32_t pid;
    char name[16]; ///< the file name, also the thread's name in "tasks"
    Memory::AddressSpace* space;
    Threading::Thread* thread;
    uint64_t entry;
    uint64_t stackTop;
    bool exited;
    int64_t exitCode;
};

class Runtime {
public:
    static constexpr uint32_t maxProcesses = 8;

    /// Top of every program's stack: one 2 MB page just below userEnd
    static constexpr uint64_t stackTop = 0x00007FFFFFFFFFF0ULL;

    Runtime(Memory::PhysicalMemoryManager& pmm, Memory::AddressSpace& kernelSpace, Threading::Scheduler& scheduler,
        Fs::Vfs& vfs);

    Runtime(const Runtime&) = delete;
    Runtime& operator=(const Runtime&) = delete;

    /**
     * @brief Load the ELF file at path into a new address space and start it
     * on a new thread (it runs at the caller's next yield).
     * @return The pid, or -1 (the reason is printed).
     */
    int spawn(const char* path);

    /// Free what exited programs used (address space, thread). Not from a program's thread.
    void reap();

    /// The program of the running thread, nullptr on a kernel thread.
    Process* current();

    const Process* find(uint32_t pid) const;
    uint32_t count() const { return m_count; }
    const Process& at(uint32_t index) const { return *m_processes[index]; }

    /// Handle int 0x80: the call's number and arguments are in frame, the result goes to frame.rax.
    void systemCall(InterruptFrame& frame);

    /// End the running program (exit, or a fault in ring 3). Does not return.
    [[noreturn]] void exitCurrent(int64_t code);

private:
    static void threadEntry(Threading::Scheduler& scheduler);
    bool load(const char* path, Process& process);
    int64_t write(Process& process, uint64_t fd, uint64_t buffer, uint64_t size);

    Memory::PhysicalMemoryManager& m_pmm;
    Memory::AddressSpace& m_kernelSpace;
    Threading::Scheduler& m_scheduler;
    Fs::Vfs& m_vfs;

    Process* m_processes[maxProcesses] {};
    uint32_t m_count {};
    uint32_t m_nextPid { 1 };
};

/**
 * @brief Bind the runtime the interrupt handlers use for int 0x80 and for
 * faults in ring 3: they are reached from assembly stubs that can not be
 * handed an object. A boundary global, like Handlers::setInterruptController.
 */
void setRuntime(Runtime* runtime);

/// The bound runtime, nullptr if there is none.
Runtime* runtime();

} // namespace Process
