// What runs between _start (crt0.S) and main: read the start stack, set up
// the heap the kernel mapped, run main, exit with what it returns.

#include "atlos.h"
#include "heap_glue.hpp"

extern "C" int main(int argc, char** argv);

// The (type, value) pairs of the auxiliary vector, see kernel/proc/process.hpp
static unsigned long auxiliaryValue(const unsigned long* auxv, unsigned long type)
{
    for (; auxv[0] != AT_NULL; auxv += 2) {
        if (auxv[0] == type)
            return auxv[1];
    }
    return 0;
}

extern "C" [[noreturn]] void atlos_start(unsigned long* stack)
{
    // argc, argv[0 .. argc - 1], NULL, envp[...], NULL, auxv
    int argc = (int)stack[0];
    char** argv = reinterpret_cast<char**>(stack + 1);
    char** envp = argv + argc + 1;
    while (*envp)
        envp++;
    const unsigned long* auxv = reinterpret_cast<const unsigned long*>(envp + 1);

    atlos_heap_init(auxiliaryValue(auxv, AT_ATLOS_HEAP_BASE), auxiliaryValue(auxv, AT_ATLOS_HEAP_SIZE),
        auxiliaryValue(auxv, AT_PAGESZ));
    exit(main(argc, argv));
}

// A pure virtual function was called (a bug): the compiler points vtables
// of abstract classes here
extern "C" void __cxa_pure_virtual()
{
    print("pure virtual function called\n");
    exit(134);
}
