# Third party code

atlOS itself is MIT licensed (see [LICENSE](LICENSE)). It includes:

| Code | Where | Origin | License |
|---|---|---|---|
| printf | `libc/stdio/printf.cpp` | Marco Paland, [mpaland/printf](https://github.com/mpaland/printf) | MIT, notice in the file |

It is kept as upstream wrote it (no reformatting or renaming).

The multiboot headers `kernel/arch/x86_64/boot/multiboot.inc` and
`multiboot2.inc` come from GNU GRUB (Copyright Free Software Foundation, Inc.)
under the permissive notice kept at the top of each file.
