/*
 * The atlOS system calls for user programs, and the heap (malloc, free). A
 * system call is "int $0x80" with the number in rax, arguments in rdi, rsi,
 * rdx, the result in rax (negative: an error). See kernel/proc/process.hpp.
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

/*
 * The auxiliary vector: what the kernel tells the program at its start
 * (kernel/proc/process.hpp). The start code reads it before main.
 */
enum {
    AT_NULL = 0,
    AT_PAGESZ = 6, /* bytes per page: 2 MB */
    AT_ATLOS_HEAP_BASE = 0x1000, /* where the heap starts */
    AT_ATLOS_HEAP_SIZE = 0x1001, /* its size: "run -heap MB", 2 MB by default */
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

/* A number in hex, 16 digits: an address */
static inline void print_hex(unsigned long value)
{
    char text[18] = "0x";
    for (int i = 0; i < 16; i++)
        text[2 + i] = "0123456789abcdef"[(value >> (60 - 4 * i)) & 0xf];
    write(STDOUT, text, sizeof(text));
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

/*
 * The heap (user/lib/malloc.cpp): the region the kernel mapped for the
 * program, divided up by the same free list the kernel uses for kmalloc.
 * Blocks are 16 byte aligned; malloc returns NULL when the heap is full.
 * free of a pointer malloc did not return ends the program (exit code 134).
 */
#ifdef __cplusplus
extern "C" {
#endif

void* malloc(unsigned long size);
void* calloc(unsigned long count, unsigned long size);
void* realloc(void* block, unsigned long size);
void free(void* block);

/* Where the heap is, how large, and how many bytes malloc has handed out */
void heap_info(unsigned long* base, unsigned long* size, unsigned long* in_use);

#ifdef __cplusplus
}
#endif
