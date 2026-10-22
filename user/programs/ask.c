/*
 * Reads a line from the keyboard and answers: shows read(0), which is raw,
 * so the program echoes what it reads and handles Backspace itself.
 */
#include "atlos.h"

// One line into line (at most size - 1 characters, then '\0'); Enter ends it
static unsigned long readLine(char* line, unsigned long size)
{
    unsigned long length = 0;
    for (;;) {
        char keys[16];
        long count = read(STDIN, keys, sizeof(keys));
        for (long i = 0; i < count; i++) {
            char c = keys[i];
            if (c == '\n') {
                print("\n");
                line[length] = '\0';
                return length;
            }
            if (c == 0x7f || c == '\b') {
                if (length > 0) {
                    length--;
                    print("\b \b"); // back, blank, back: the character goes from the screen
                }
            } else if (c >= ' ' && length < size - 1) {
                line[length++] = c;
                write(STDOUT, &c, 1);
            }
        }
    }
}

int main(void)
{
    char name[32];
    print("What is your name? ");
    if (readLine(name, sizeof(name)) == 0) {
        print("No name, no greeting.\n");
        return 1;
    }

    print("Hello, ");
    print(name);
    print("! I am process ");
    print_number(getpid());
    print(".\n");
    return 0;
}
