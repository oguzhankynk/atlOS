# Third party code

atlOS itself is MIT licensed (see [LICENSE](LICENSE)). It includes:

| Code | Where | Origin | License |
|---|---|---|---|
| liballoc 1.1 | `kernel/mm/liballoc/liballoc_1_1.*` | Durand Miller, liballoc 1.1 | public domain (as released by its author) |
| printf | `libc/stdio/printf.cpp` | Marco Paland, [mpaland/printf](https://github.com/mpaland/printf) | MIT, notice in the file |

Both are kept as upstream wrote them (no reformatting or renaming).
`kernel/mm/liballoc/liballoc_hooks.cpp` is atlOS code that connects liballoc to
the kernel.

The multiboot headers `kernel/arch/x86_64/boot/multiboot.inc` and
`multiboot2.inc` come from GNU GRUB (Copyright Free Software Foundation, Inc.)
under the permissive notice kept at the top of each file.
