# User programs (`user/`)

Programs that run in ring 3 under atlOS, see `kernel/proc/README.md`.

- `lib/crt0.S`: `_start`, calls `main` and passes its return value to `exit`
- `lib/atlos.h`: the system calls (`int $0x80`: `write`, `exit`) and a few helpers
- `user.ld`: code at `0x0000008000000000`, data in the next 2 MB page
- `programs/`: `hello.c`, `fault.c` (writes to address 0), `badptr.c`
  (pointers the kernel must refuse)

They are built with the kernel's cross compiler (`-ffreestanding -nostdlib`,
no SSE: the kernel does not save SIMD state) and copied to the test disk as
`/BIN/<NAME>.ELF` (`scripts/make-disk-image.sh`). Run one from the shell:

```
atlOS:/> run /disk0/BIN/HELLO.ELF
```

A new program: add `programs/name.c` and `atlos_user_program(name)` in
`CMakeLists.txt`.
