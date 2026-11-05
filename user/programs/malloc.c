/*
 * The heap: where the kernel put it, a linked list of numbers on it, and the
 * largest block that fits. "run -heap 8 /disk0/BIN/MALLOC.ELF" gives it
 * 8 MB instead of 2. MALLOC.ELF [n]: n numbers in the list (default 1000).
 * Exits with 0, or 1 if the list did not fit.
 */
#include "atlos.h"

struct node {
    struct node* next;
    unsigned long value;
};

static unsigned long parse(const char* text)
{
    unsigned long value = 0;
    while (*text >= '0' && *text <= '9')
        value = value * 10 + (unsigned long)(*text++ - '0');
    return value;
}

static void print_in_use(const char* when)
{
    unsigned long base, size, in_use;
    heap_info(&base, &size, &in_use);
    print(when);
    print_number(in_use);
    print(" bytes in use\n");
}

int main(int argc, char** argv)
{
    unsigned long base, size, in_use;
    heap_info(&base, &size, &in_use);
    print("heap: ");
    print_number(size / 1024);
    print(" KB at ");
    print_hex(base);
    print("\n");

    // A list that grows one node at a time: nothing here knows its length in advance
    unsigned long count = argc > 1 ? parse(argv[1]) : 1000;
    struct node* head = 0;
    unsigned long made = 0;
    for (unsigned long i = 1; i <= count; i++) {
        struct node* node = malloc(sizeof(struct node));
        if (!node)
            break;
        node->value = i;
        node->next = head;
        head = node;
        made++;
    }

    unsigned long sum = 0;
    for (struct node* node = head; node; node = node->next)
        sum += node->value;
    print("list of ");
    print_number(made);
    print(" numbers, sum ");
    print_number(sum);
    print("\n");
    print_in_use("with the list: ");

    while (head) {
        struct node* next = head->next;
        free(head);
        head = next;
    }
    print_in_use("after free: ");

    // The largest block: every block and run of pages has a header, so a
    // heap of n MB never fits a block of n MB
    unsigned long largest = 0;
    for (unsigned long mb = size >> 20; mb > 0; mb--) {
        void* block = malloc(mb << 20);
        if (block) {
            largest = mb;
            free(block);
            break;
        }
    }
    print("largest block: ");
    print_number(largest);
    print(" MB\n");

    return made == count ? 0 : 1;
}
