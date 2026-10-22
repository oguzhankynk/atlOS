/*
 * Prints the files named on its command line: argv and the file system calls
 * together. Paths start at the root ("run /disk0/BIN/CAT.ELF /disk0/HELLO.TXT").
 */
#include "atlos.h"

int main(int argc, char** argv)
{
    if (argc < 2) {
        print("usage: CAT.ELF /path/file ...\n");
        return 1;
    }

    int failed = 0;
    for (int i = 1; i < argc; i++) {
        long fd = open(argv[i], O_RDONLY);
        if (fd < 0) {
            print(argv[i]);
            print(fd == -ENOENT ? ": no such file\n" : ": can not open\n");
            failed = 1;
            continue;
        }
        char buffer[512];
        long count;
        while ((count = read(fd, buffer, sizeof(buffer))) > 0)
            write(STDOUT, buffer, count);
        close(fd);
    }
    return failed;
}
