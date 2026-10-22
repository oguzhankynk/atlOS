/**
 * @file process.hpp
 * @author Oguzhan KAYNAK
 * @brief User programs: loading a static ELF64 file into an address space of
 * its own and running it in ring 3 on a kernel thread; the int 0x80 system
 * calls (the console, files, time); ending a program on exit or on a fault.
 */

#pragma once

#include <stdint.h>

#include <arch/x86_64/cpu/cpu.hpp>
#include <sync/semaphore.hpp>

namespace Drivers {
class CharacterSource;
}

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
    SysWrite = 1, ///< write(fd, buffer, size): the screen (1, 2) or a file
    SysRead = 2, ///< read(fd, buffer, size): the keyboard (0) or a file; 0 at the end of a file
    SysOpen = 3, ///< open(path, flags): a file descriptor (3 and up)
    SysClose = 4, ///< close(fd)
    SysYield = 5, ///< yield(): let the other threads run
    SysSleep = 6, ///< sleep(ms)
    SysGetPid = 7, ///< getpid(): the program's process id
    SysUptime = 8, ///< uptime(): milliseconds since boot
};

/// open flags (the Linux values): one of the three access modes, then any of the others
enum OpenFlags : uint64_t {
    OpenReadOnly = 0, ///< O_RDONLY
    OpenWriteOnly = 1, ///< O_WRONLY
    OpenReadWrite = 2, ///< O_RDWR
    OpenAccessMode = 3, ///< the bits of the access mode
    OpenCreate = 0x40, ///< O_CREAT: create the file if there is none
    OpenTruncate = 0x200, ///< O_TRUNC: start the file empty
    OpenAppend = 0x400, ///< O_APPEND: every write goes to the end
};

/// Negative results of a system call (the Linux numbers)
enum SystemCallError : int64_t {
    ErrorNoEntry = -2, ///< ENOENT: no such file
    ErrorIo = -5, ///< EIO: the file system refused (disk full, device error)
    ErrorBadFile = -9, ///< EBADF: not an open file descriptor, or not open for this
    ErrorFault = -14, ///< EFAULT: a pointer the program may not use
    ErrorIsDirectory = -21, ///< EISDIR
    ErrorInvalid = -22, ///< EINVAL
    ErrorTooManyFiles = -24, ///< EMFILE: every file descriptor is in use
    ErrorNameTooLong = -36, ///< ENAMETOOLONG
    ErrorNoSystemCall = -38, ///< ENOSYS
};

/// A file the program opened: where it is and how far it has read or written
struct OpenFile {
    static constexpr uint32_t maxPath = 128;

    bool used;
    bool readable;
    bool writable;
    bool append;
    uint64_t offset;
    char path[maxPath];
};

struct Process {
    /// File descriptors 0 (keyboard), 1 and 2 (screen) are the console; files get 3 and up
    static constexpr uint32_t maxOpenFiles = 8;
    static constexpr int64_t firstFileDescriptor = 3;

    uint32_t pid;
    char name[16]; ///< the file name, also the thread's name in "tasks"
    Memory::AddressSpace* space;
    Threading::Thread* thread;
    uint64_t entry;
    uint64_t stackTop;
    bool exited;
    int64_t exitCode;
    Sync::Semaphore done { 0, 1 }; ///< given on exit: Runtime::wait takes it
    OpenFile files[maxOpenFiles]; ///< files[i] is file descriptor firstFileDescriptor + i
};

class Runtime {
public:
    static constexpr uint32_t maxProcesses = 8;

    /// Top of every program's stack: one 2 MB page just below userEnd
    static constexpr uint64_t stackTop = 0x00007FFFFFFFFFF0ULL;

    /// Programs run on the caller's scheduler (Threading::currentScheduler()) when spawned
    /// and read the keyboard (file descriptor 0) from input.
    Runtime(Memory::PhysicalMemoryManager& pmm, Memory::AddressSpace& kernelSpace, Fs::Vfs& vfs,
        Drivers::CharacterSource& input);

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

    /**
     * @brief Sleep until the program pid has exited, then free it.
     * @return Its exit code, or -1 if there is no such program.
     */
    int64_t wait(uint32_t pid);

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
    static void threadEntry();
    bool load(const char* path, Process& process);
    int64_t write(Process& process, uint64_t fd, uint64_t buffer, uint64_t size);
    int64_t read(Process& process, uint64_t fd, uint64_t buffer, uint64_t size);
    int64_t readConsole(char* buffer, uint64_t size);
    int64_t open(Process& process, uint64_t path, uint64_t flags);
    int64_t close(Process& process, uint64_t fd);
    static OpenFile* openFile(Process& process, uint64_t fd);

    Memory::PhysicalMemoryManager& m_pmm;
    Memory::AddressSpace& m_kernelSpace;
    Fs::Vfs& m_vfs;
    Drivers::CharacterSource& m_input;

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
