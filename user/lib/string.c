/*
 * The four functions a C compiler may call on its own, even in a
 * freestanding program: to copy or clear a large struct, or for a loop it
 * recognizes (the heap's byte loops). Built with
 * -fno-tree-loop-distribute-patterns, so these loops are not turned into
 * calls to themselves.
 */

void* memcpy(void* to, const void* from, unsigned long count)
{
    unsigned char* d = to;
    const unsigned char* s = from;
    for (unsigned long i = 0; i < count; i++)
        d[i] = s[i];
    return to;
}

void* memmove(void* to, const void* from, unsigned long count)
{
    unsigned char* d = to;
    const unsigned char* s = from;
    if (d < s) {
        for (unsigned long i = 0; i < count; i++)
            d[i] = s[i];
    } else {
        for (unsigned long i = count; i > 0; i--)
            d[i - 1] = s[i - 1];
    }
    return to;
}

void* memset(void* to, int value, unsigned long count)
{
    unsigned char* d = to;
    for (unsigned long i = 0; i < count; i++)
        d[i] = (unsigned char)value;
    return to;
}

int memcmp(const void* a, const void* b, unsigned long count)
{
    const unsigned char* x = a;
    const unsigned char* y = b;
    for (unsigned long i = 0; i < count; i++) {
        if (x[i] != y[i])
            return x[i] < y[i] ? -1 : 1;
    }
    return 0;
}
