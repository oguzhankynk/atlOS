# Writing user programs for atlOS (`user/`)

User programs are small C programs that atlOS loads from its disk and runs in
**ring 3**, in an address space of their own: they can not touch the kernel,
and when they crash, only they end. This directory has everything needed to
write one; how the kernel side works is in `kernel/proc/README.md`.

```
atlOS:/> run /disk0/BIN/COUNT.ELF
1 2 3 4 5
[pid 1 COUNT.ELF exited with 15]
atlOS:/>
```

## What is here

```
user/
  CMakeLists.txt       builds every program: atlos_user_program(name)
  user.ld              linker script: where code, data and the entry point go
  lib/crt0.S           _start: calls main(), passes its result to exit
  lib/atlos.h          the system calls and a few helpers
  programs/            the programs, one .c file each
    hello.c            prints, exits with 0
    count.c            the example below
    fault.c            writes to address 0: ends with a page fault
    badptr.c           hands the kernel pointers it must refuse
    break.c            stops at a breakpoint, then goes on
    ask.c              reads a line from the keyboard (read, echo, Backspace)
    clock.c            uptime, sleep and yield
    files.c            open, read, write, close and their errors
```

## The environment

What a program can count on, and what it can not (yet):

| | |
|---|---|
| Language | C, freestanding (`-ffreestanding -nostdlib`), built with the kernel's cross compiler `x86_64-elf-gcc` |
| Library | **no libc**: no `printf`, `malloc`, `string.h`. `atlos.h` has the system calls and `print`, `print_number`, `strlen` |
| Start | `_start` (crt0) calls `int main(void)`; its return value becomes the exit code. No `argc`/`argv`, no environment |
| Registers at start | all zero; `rsp` 16 byte aligned |
| Floating point | **none**: the kernel does not save SSE state, programs are built with `-mno-sse`; using `float`/`double` fails to compile (`SSE register return with SSE disabled`) |
| Input | `read(STDIN, ...)`: the keys, raw (see below) |
| Files | `open`, `read`, `write`, `close`; paths from the root (`/disk0/NOTE.TXT`), 8.3 names, up to 8 open at once |
| Time | `uptime()` (milliseconds since boot), `sleep(ms)`, `yield()` |
| Several programs | up to 8 at once, each in its own address space at the same addresses |

### Memory

Every program gets the same layout (2 MB pages, see `user.ld`):

| Address | What | Access |
|---|---|---|
| `0x0000008000000000` | code (`.text`, `.rodata`), starting right after the ELF headers | read, execute |
| next 2 MB page | data (`.data`, `.bss`) | read, write |
| `0x00007FFFFFE00000` .. `0x00007FFFFFFFFFF0` | the stack: one 2 MB page, `rsp` starts at the top | read, write |

Every page is zeroed before the program starts, so `.bss` and unused stack
are 0. There is no heap: use globals and the stack. The 2 MB stack is the only
stack page; running off its bottom is a page fault.

Everything else faults: the kernel half, and the first megabytes (identity
mapped for the kernel only), so **dereferencing `NULL` is a page fault**, as it
should be.

### Cooperative scheduling: give the CPU back

atlOS has no preemption. A program keeps the CPU until it exits, or until
it calls `yield`, `sleep` or a `read` that has to wait for a key; the other
system calls return to it right away. A program that loops forever without
one of those **freezes the whole machine**, the shell included. Keep
programs short, and make sure they end or wait.

`run` waits until the program exits, then the shell prompts again.

## System calls

`int $0x80`, the call number in `rax`, arguments in `rdi`, `rsi`, `rdx`, the
result in `rax`; a negative result is an error. `atlos.h` wraps them:

| rax | Call | Wrapper | Result |
|---|---|---|---|
| 0 | exit | `exit(code)` | does not return |
| 1 | write | `write(fd, buffer, size)` | bytes written; `-EBADF` (not open for writing), `-EINVAL` above 4096 bytes, `-EFAULT` if the buffer is not the program's own memory |
| 2 | read | `read(fd, buffer, size)` | bytes read, 0 at the end of a file; the same errors |
| 3 | open | `open(path, flags)` | a file descriptor, 3 and up; `-ENOENT`, `-EISDIR`, `-EMFILE` (8 open already), `-EFAULT` |
| 4 | close | `close(fd)` | 0, or `-EBADF` |
| 5 | yield | `yield()` | the other threads run first |
| 6 | sleep | `sleep(ms)` | at least `ms` milliseconds later |
| 7 | getpid | `getpid()` | the process id (the shell's `[pid 3 ...]`) |
| 8 | uptime | `uptime()` | milliseconds since boot |
| other | | `atlos_syscall(n, a, b, c)` | `-ENOSYS` |

The errors are the Linux numbers, negated: `if (fd == -ENOENT)`. File
descriptors 0, 1 and 2 are `STDIN`, `STDOUT` and `STDERR`. `open` takes one
of `O_RDONLY`, `O_WRONLY`, `O_RDWR`, and any of `O_CREAT` (create the file),
`O_TRUNC` (empty it), `O_APPEND` (write at the end):

```c
long fd = open("/disk0/NOTE.TXT", O_WRONLY | O_CREAT | O_TRUNC);
write(fd, "hello\n", 6);
close(fd);
```

### Reading the keyboard

`read(STDIN, buffer, size)` waits for a key, then returns it and the keys
typed after it, at least one. Nothing is echoed and there is no line
editing: the program sees every key as it comes and prints what it wants
(`ask.c` echoes the keys and handles Backspace). Enter arrives as `'\n'`,
Backspace as `0x7f` (a terminal) or `'\b'`. **Ctrl+C** ends the program
with exit code 130.

`print(text)` is `write(STDOUT, text, strlen(text))`, `print_number(n)`
prints a number in decimal. `breakpoint()` executes
`int3`: the kernel prints the program's registers and the program goes on
(`BREAK.ELF`).

The kernel checks every pointer it is given: the buffer of `write` must lie
in the program's own mapped pages. Passing a kernel address does not leak
kernel memory, it returns `-EFAULT` (`badptr.c` tries several such pointers).

## Exit codes

The shell shows how a program ended: `[pid 2 FAULT.ELF exited with 142]`.

| Exit code | Meaning |
|---|---|
| what `main` returned, or the argument of `exit` | normal end |
| 128 + exception vector | the program faulted and the kernel ended it |
| 130 | Ctrl+C while the program read the keyboard |
| 128 (#DE) | division by zero |
| 134 (#UD) | invalid instruction |
| 141 (#GP) | general protection: a privileged instruction (`cli`, `hlt`, `in`/`out`, `mov cr3`), `int` with another vector than `0x80`, a non canonical address |
| 142 (#PF) | page fault: `NULL`, kernel memory, writing to code, beyond the stack |

## Your first program

1. Write `programs/count.c`:

   ```c
   #include "atlos.h"

   static long s_sum; // .bss: starts at 0

   // No printf: turn a number into text by hand
   static void printNumber(long value)
   {
       char text[24];
       int length = 0;
       do {
           text[length++] = (char)('0' + value % 10);
           value /= 10;
       } while (value);

       char reversed[24];
       for (int i = 0; i < length; i++)
           reversed[i] = text[length - 1 - i];
       write(1, reversed, length);
   }

   int main(void)
   {
       for (long i = 1; i <= 5; i++) {
           printNumber(i);
           print(i < 5 ? " " : "\n");
           s_sum += i;
       }
       return s_sum; // 15: shown as "exited with 15"
   }
   ```

2. Add it to `CMakeLists.txt` (both lines):

   ```cmake
   atlos_user_program(count)
   set(ATLOS_USER_TARGETS hello fault badptr count break ask clock files PARENT_SCOPE)
   ```

   The target name becomes the file name on the disk, in upper case:
   `count` → `/BIN/COUNT.ELF`. FAT16 names are 8.3, so keep it to 8
   characters.

3. Build the kernel, the programs and the test disk, and boot it:

   ```sh
   scripts/build.sh all
   cmake --build --preset default --target disk-image   # build/default/disk.img
   scripts/build.sh run                                   # QEMU, the shell on this terminal
   ```

   `scripts/build.sh run` attaches the disk. The program itself is
   `build/default/user/count.elf`.

4. Run it in the shell:

   ```
   atlOS:/> ls /disk0/BIN
     HELLO.ELF    ...
     FAULT.ELF    ...
     BADPTR.ELF   ...
     COUNT.ELF    ...
   atlOS:/> run /disk0/BIN/COUNT.ELF
   ```

## When it crashes

A fault in a program prints the CPU's report, then ends the program:

```
==== EXCEPTION 14: Page fault (in user mode) ====
error code 0x7
CR2=0x0000000000000000 (user write, protection violation)
...
RIP=0x00000080000000c4 CS=0x001b RFLAGS=0x0000000000000246
[pid 2 FAULT.ELF exited with 142]
```

- `CR2` is the address the program touched (page faults only), and the
  decoded error code says how: a user *write*, to a page that is mapped but
  not the program's (*protection violation*) or to no page at all (*page not
  present*).
- `RIP` is the instruction. Turn it into a source line with the program's ELF
  file (programs are built with `-g`), feeding it the serial log or the pasted
  report:

  ```sh
  scripts/symbolize.sh build/default/user/fault.elf build/default/shelltest.log
  RIP 0x00000080000000c4 main at user/programs/fault.c:12
  ```

  or by hand: `x86_64-elf-addr2line -f -e build/default/user/fault.elf 0x80000000c4`.
- To see the instructions: `x86_64-elf-objdump -d build/default/user/fault.elf`.
- To see the segments the kernel loads: `x86_64-elf-readelf -l build/default/user/fault.elf`.

A program that the kernel refuses to load says why, for example
`... two segments share a 2 MB page (link with -z max-page-size=0x200000)`
or `... a segment is outside the user address range`.

## Rules for the build

Keep these when changing `CMakeLists.txt` or `user.ld`; the loader
(`kernel/proc/elf.cpp`) refuses files that break them:

- static ELF64 executables (`-static -nostdlib`, no PIE), for x86_64;
- every loaded segment inside `0x0000008000000000` .. `0x0000800000000000`;
- code and data in **different 2 MB pages**: protection is per page. The
  linker script starts data on the next page, and
  `-z max-page-size=0x200000` keeps the file small (the offsets inside the
  file match the addresses modulo 2 MB);
- no SSE (`-mno-sse -mno-sse2 -mno-mmx`), `-mcmodel=large` (the code is far
  above 2 GB).

## Testing a program

- The **shell test** (`scripts/build.sh shelltest`, `scripts/run-shelltest.py`)
  boots the kernel, types `run /disk0/BIN/...` and checks the output; add a
  step there for a program whose output matters.
- The **self tests** (`kernel/tests/selftest.cpp`, *User programs*) run
  programs from the test disk with `runProgram(runtime, scheduler, path)` and
  check exit codes and output; they also check that every page comes back.

## References

- `kernel/proc/README.md`: loading, ring 3, system calls, from the kernel's side
- System V ABI, x86-64 supplement: calling convention, process start
- OSDev Wiki: [ELF](https://wiki.osdev.org/ELF), [System Calls](https://wiki.osdev.org/System_Calls)
