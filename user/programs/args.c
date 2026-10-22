/*
 * Prints its arguments and where they are: the kernel put the strings and
 * argv[] at the top of the stack. Exits with argc.
 */
#include "atlos.h"

int main(int argc, char** argv)
{
    print("argc = ");
    print_number(argc);
    print("\n");

    for (int i = 0; i < argc; i++) {
        print("argv[");
        print_number(i);
        print("] = \"");
        print(argv[i]);
        print("\" at ");
        print_hex((unsigned long)argv[i]);
        print("\n");
    }

    // The list ends with a NULL, as in every Unix
    print(argv[argc] == 0 ? "argv[argc] = NULL\n" : "argv[argc] is not NULL\n");
    return argc;
}
