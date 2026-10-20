/*
 * The atlOS system calls for user programs: "int $0x80" with the number in
 * rax, arguments in rdi, rsi, rdx, the result in rax (negative: an error).
 * See kernel/proc/process.hpp.
 */
#pragma once

enum {
    SYS_EXIT = 0,
    SYS_WRITE = 1,
};

enum {
    EBADF = 9,
    EFAULT = 14,
    EINVAL = 22,
    ENOSYS = 38,
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

static inline long write(int fd, const void* buffer, unsigned long size)
{
    return atlos_syscall(SYS_WRITE, fd, (long)buffer, (long)size);
}

__attribute__((noreturn)) static inline void exit(long code)
{
    atlos_syscall(SYS_EXIT, code, 0, 0);
    for (;;) {
    }
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
    return write(1, s, strlen(s));
}
