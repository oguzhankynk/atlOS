/*
 * The file system calls: writes /disk0/USER.TXT, appends to it, reads it
 * back, then checks the errors. Exits with 0 if everything went as expected.
 */
#include "atlos.h"

static const char* const path = "/disk0/USER.TXT";

static int s_wrong;

static void expect(int ok, const char* what)
{
    if (!ok) {
        print("files: wrong: ");
        print(what);
        print("\n");
        s_wrong++;
    }
}

int main(void)
{
    // Create (or empty) the file and write a line
    long fd = open(path, O_WRONLY | O_CREAT | O_TRUNC);
    expect(fd >= 3, "open for writing");
    expect(write(fd, "first line\n", 11) == 11, "write");
    expect(close(fd) == 0, "close");

    // O_APPEND: the write goes to the end
    fd = open(path, O_WRONLY | O_APPEND);
    expect(write(fd, "second line\n", 12) == 12, "append");
    close(fd);

    // Read it back in small pieces: read moves on, 0 at the end
    char text[64];
    unsigned long length = 0;
    fd = open(path, O_RDONLY);
    for (long count; (count = read(fd, text + length, 5)) > 0;)
        length += count;
    close(fd);
    expect(length == 23, "read back 23 bytes");
    print("files: ");
    write(STDOUT, text, length);

    // What must be refused
    expect(open("/disk0/NOPE.TXT", O_RDONLY) == -ENOENT, "a missing file");
    expect(open("USER.TXT", O_RDONLY) == -ENOENT, "a relative path");
    expect(open("/disk0/BIN", O_RDONLY) == -EISDIR, "a directory");
    expect(open((const char*)0xFFFFFFFF80000000UL, O_RDONLY) == -EFAULT, "a path in the kernel");
    fd = open(path, O_RDONLY);
    expect(write(fd, "x", 1) == -EBADF, "writing a file open for reading");
    close(fd);
    expect(close(fd) == -EBADF, "closing twice");

    // Every file descriptor in use: the next open fails
    long fds[8];
    for (int i = 0; i < 8; i++)
        fds[i] = open(path, O_RDONLY);
    expect(open(path, O_RDONLY) == -EMFILE, "too many open files");
    for (int i = 0; i < 8; i++)
        close(fds[i]);

    print(s_wrong ? "files: something went wrong\n" : "files: all as expected\n");
    return s_wrong;
}
