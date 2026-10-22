/*
 * The atlOS system calls for user programs: "int $0x80" with the number in
 * rax, arguments in rdi, rsi, rdx, the result in rax (negative: an error).
 * See kernel/proc/process.hpp.
 */
#pragma once

enum {
    SYS_EXIT = 0,
    SYS_WRITE = 1,
    SYS_READ = 2,
    SYS_OPEN = 3,
    SYS_CLOSE = 4,
    SYS_YIELD = 5,
    SYS_SLEEP = 6,
    SYS_GETPID = 7,
    SYS_UPTIME = 8,
};

/* The errors, returned negated: write(...) == -EBADF */
enum {
    ENOENT = 2,
    EIO = 5,
    EBADF = 9,
    EFAULT = 14,
    EISDIR = 21,
    EINVAL = 22,
    EMFILE = 24,
    ENAMETOOLONG = 36,
    ENOSYS = 38,
};

/* open flags: one access mode, then any of the others (the Linux values) */
enum {
    O_RDONLY = 0,
    O_WRONLY = 1,
    O_RDWR = 2,
    O_CREAT = 0x40,
    O_TRUNC = 0x200,
    O_APPEND = 0x400,
};

/* The console's file descriptors; open hands out 3 and up */
enum {
    STDIN = 0, /* the keyboard (and the serial terminal) */
    STDOUT = 1, /* the screen (and the serial terminal) */
    STDERR = 2, /* the same screen */
};

static inline long atlos_syscall(long number, long a, long b, long c)
{
    long result;
    __asm__ volatile("int $0x80"
                     : "=a"(result)
                     : "a"(number), "D"(a), "S"(b), "d"(c)
                     : "memory");
    return result;
}

/* fd 1 or 2: the screen; a file: at its position, or at its end with O_APPEND */
static inline long write(int fd, const void* buffer, unsigned long size)
{
    return atlos_syscall(SYS_WRITE, fd, (long)buffer, (long)size);
}

/*
 * fd 0: waits for a key, then returns the keys typed so far (at least 1).
 * Raw: nothing is echoed, Enter is '\n', Backspace is 0x7f or 0x08, and
 * Ctrl+C ends the program (exit code 130). A file: 0 at its end.
 */
static inline long read(int fd, void* buffer, unsigned long size)
{
    return atlos_syscall(SYS_READ, fd, (long)buffer, (long)size);
}

/* path from the root ("/disk0/NOTE.TXT"): a file descriptor, or -ENOENT, ... */
static inline long open(const char* path, int flags)
{
    return atlos_syscall(SYS_OPEN, (long)path, flags, 0);
}

static inline long close(int fd)
{
    return atlos_syscall(SYS_CLOSE, fd, 0, 0);
}

/* Give the CPU to the other threads for a moment */
static inline void yield(void)
{
    atlos_syscall(SYS_YIELD, 0, 0, 0);
}

static inline void sleep(unsigned long ms)
{
    atlos_syscall(SYS_SLEEP, (long)ms, 0, 0);
}

static inline long getpid(void)
{
    return atlos_syscall(SYS_GETPID, 0, 0, 0);
}

/* Milliseconds since boot */
static inline unsigned long uptime(void)
{
    return (unsigned long)atlos_syscall(SYS_UPTIME, 0, 0, 0);
}

__attribute__((noreturn)) static inline void exit(long code)
{
    atlos_syscall(SYS_EXIT, code, 0, 0);
    for (;;) {
    }
}

/* Stop for a moment: the kernel prints the registers, then the program goes on */
static inline void breakpoint(void)
{
    __asm__ volatile("int3" ::
                         : "memory");
}

static inline unsigned long strlen(const char* s)
{
    unsigned long n = 0;
    while (s[n])
        n++;
    return n;
}

static inline long print(const char* s)
{
    return write(STDOUT, s, strlen(s));
}

/* A number in decimal (there is no printf) */
static inline void print_number(unsigned long value)
{
    char text[24];
    int length = sizeof(text);
    do {
        text[--length] = (char)('0' + value % 10);
        value /= 10;
    } while (value);
    write(STDOUT, text + length, sizeof(text) - length);
}
